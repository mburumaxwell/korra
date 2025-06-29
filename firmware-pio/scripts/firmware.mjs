#!/usr/bin/env node

import chalk from 'chalk';
import { Command, Option } from 'commander';
import { execSync, spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import { mkdir, copyFile, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import process from 'node:process';
import readline from 'node:readline';
import * as semver from 'semver';

import packageJson from '../package.json' with { type: "json" };

const isCI = Boolean(process.env['CI'] || process.env['GITHUB_ACTIONS']);
const KNOWN_ENVIRONMENTS = [
  'keeper-esp32-s3-devkitc',
  'pot-esp32-s3-devkitc',
];

const version = new Command('version')
  .description('Generate VERSION file for PlatformIO.')
  .option('--dev', 'Whether in dev mode.')
  .action(async (...args) => {
    const [options/*, command*/] = args;
    const { dev } = options;

    let version = semver.parse(packageJson.version);
    if (!version) {
      throw new Error('Invalid version in package.json');
    }

    const contents = `${version}\n`
    const destination = join(process.cwd(), './version.txt')
    if (existsSync(destination)) await rm(destination);
    await writeFile(destination, contents, 'utf-8');
    console.log(`✅ Generated VERSION file for ${version} at ${destination}`);
  });

const build = new Command('build')
  .description('Build firmware.')
  .addOption(new Option('--environment [environment...]', 'The target(s) to build.').choices(KNOWN_ENVIRONMENTS).default(KNOWN_ENVIRONMENTS))
  .action(async (...args) => {
    const [options/*, command*/] = args;
    const { environment: environments } = options;

    const colors = [
      chalk.cyan,
      chalk.magenta,
      chalk.yellow,
      chalk.green,
      chalk.blue,
      chalk.white,
      chalk.gray,
    ];
    let colorIndex = 0;

    const procs = [];
    const childProcs = [];
    const results = []; // NEW: keep success/failure per task
    const startTime = Date.now(); // NEW: track elapsed time

    ['SIGINT', 'SIGTERM'].forEach(signal => {
      process.on(signal, () => {
        childProcs.forEach((cp) => cp.kill('SIGINT'));
        process.exit(1);
      });
    });

    for (const environment of environments) {
      const color = colors[(colorIndex++) % colors.length];
      const id = `[${environment}]`;
      const prefix = !isCI ? color(id) : id;

      // The --pristine/-p option takes: auto|always|never
      // Passing --pristine/-p is equivalent to passing 'always' value.
      // Docs: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#pristine-builds
      // If we include any --extra* option, we force pristine but mostly in dev, we need to save time
      // The --extra* options are only passed if we have pristine which means by choice
      // Scenarios where passing --pristine makes sense is in CI where the extra files are not already known
      // or in dev when we add a new file or feel there is a need for it
      const cmd = [
        'pio run',
        `--environment ${environment}`,
        '--project-dir ../'
      ].filter(Boolean).join(' ');

      const proc = spawn(cmd, { shell: true });
      childProcs.push(proc);
      procs.push(new Promise((resolve) => {
        const procStart = Date.now();

        const finish = (code) => {
          const success = code === 0;
          const elapsed = ((Date.now() - procStart) / 1000).toFixed(1);
          results.push({ id, success, duration: elapsed });
          resolve();
        };

        if (isCI) {
          console.log(`::group::Build ${prefix}`);
          console.log(`${prefix} 🏗️ Running: ${cmd}`);
          // Buffer mode for CI
          let stdout = '';
          let stderr = '';

          proc.stdout.on('data', data => { stdout += data.toString(); });
          proc.stderr.on('data', data => { stderr += data.toString(); });

          proc.on('close', (code) => {
            console.log(stdout.trim());
            console.error(stderr.trim());
            console.log(`::endgroup::`);
            finish(code);
          });
        } else {
          // Local: stream with prefix
          console.log(`${prefix} 🏗️ Running: ${cmd}`);
          const rlOut = readline.createInterface({ input: proc.stdout });
          rlOut.on('line', line => console.log(`${prefix} ${line}`));

          const rlErr = readline.createInterface({ input: proc.stderr });
          rlErr.on('line', line => console.error(`${prefix} ${line}`));

          proc.on('close', (code) => {
            finish(code);
          });
        }
      }));
    }

    await Promise.all(procs);

    const duration = ((Date.now() - startTime) / 1000).toFixed(1);
    const succeeded = results.filter(r => r.success);
    const failed = results.filter(r => !r.success);
    console.log(`\n📊 Build Summary (${duration}s):`);
    succeeded.forEach(r => console.log(`✅ ${r.id} (${r.duration}s)`));
    failed.forEach(r => console.log(`❌ ${r.id} (${r.duration}s)`));

    if (failed.length > 0) {
      process.exit(1);
    } else {
      console.log(`\n🚀 All builds succeeded.`);
    }
  });

const flash = new Command('flash')
  .description('Flash firmware.')
  .addOption(new Option('--environment <environment>', 'The target to flash.').choices(KNOWN_ENVIRONMENTS).makeOptionMandatory())
  .action(async (...args) => {
    const [options/*, command*/] = args;
    const { environment } = options;
    console.log(`🚀 Flashing ${environment}`);

    const cmd = [
      'pio run',
      `--environment ${environment}`,
      '--target flash',
      '--project-dir ../'
    ].filter(Boolean).join(' ');
    console.log(`🏗️ Running: ${cmd}`);

    try {
      execSync(cmd, { stdio: 'inherit' });
      console.log(`✅ Flashing ${environment} completed`);
    } catch (error) {
      console.error(`❌ Flash failed:`, error.message);
      process.exit(1);
    }
  });

const collect = new Command('collect')
  .description('Collect firmware binaries.')
  .addOption(new Option('--environment [environment...]', 'The target(s) to build.').choices(KNOWN_ENVIRONMENTS).default(KNOWN_ENVIRONMENTS))
  .action(async (...args) => {
    const [options/*, command*/] = args;
    const { environment: environments } = options;
    console.log(`📦 Collecting build artifacts ...`);
    const outputDir = 'binaries';
    if (existsSync(outputDir)) await rm(outputDir, { recursive: true });
    await mkdir(outputDir, { recursive: true });

    for (const environment of environments) {
      const buildDir = `.pio//build/${environment}`;
      const binSrc = join(buildDir, 'firmware.bin');
      const elfSrc = join(buildDir, 'firmware.elf');
      const binDest = join(outputDir, `arduino-${environment}.bin`);
      const elfDest = join(outputDir, `arduino-${environment}.elf`);

      console.log(`🔹 Copying ${binSrc} → ${binDest}`);
      await copyFile(binSrc, binDest);

      console.log(`🔹 Copying ${elfSrc} → ${elfDest}`);
      await copyFile(elfSrc, elfDest);
    }

    console.log(`✅ Done collecting binaries in ${outputDir}/`);
  });

const root = new Command();
root.name('korra-firmware').description('CLI too for running firmware tasks for Korra.');
root.usage();
root.version(packageJson.version, '--version');
root.addCommand(version);
root.addCommand(build);
root.addCommand(flash);
root.addCommand(collect);

const args = process.argv;
root.parse(args);

// If no command is provided, show help
if (!args.slice(2).length) {
  root.help();
}
