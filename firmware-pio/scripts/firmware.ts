#!/usr/bin/env node

import chalk from 'chalk';
import { Command, Option } from 'commander';
import { type ChildProcessWithoutNullStreams, execSync, spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import { copyFile, mkdir, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import process from 'node:process';
import readline from 'node:readline';
import * as semver from 'semver';

import packageJson from '../package.json' with { type: 'json' };

const isCI = Boolean(process.env['CI'] || process.env['GITHUB_ACTIONS']);
const KNOWN_ENVIRONMENTS = [
  'arduino-keeper-esp32s3_devkitc',
  'arduino-pot-esp32s3_devkitc',

  // 'espidf-keeper-esp32s3_devkitc',
  // 'espidf-pot-esp32s3_devkitc',

  // 'zephyr-keeper-esp32s3_devkitc',
  // 'zephyr-pot-esp32s3_devkitc',

  // add other board/framework targets here
];

const version = new Command('version')
  .description('Generate VERSION file for PlatformIO.')
  .option('--dev', 'Whether in dev mode.')
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { dev } = options;

    let version = semver.parse(packageJson.version);
    if (!version) {
      throw new Error('Invalid version in package.json');
    }

    const contents = `${version}\n`;
    const destination = join(process.cwd(), './version.txt');
    if (existsSync(destination)) await rm(destination);
    await writeFile(destination, contents, 'utf-8');
    console.log(`✅ Generated VERSION file for ${version} at ${destination}`);
  });

const build = new Command('build')
  .description('Build firmware.')
  .addOption(
    new Option('--environment [environment...]', 'The target(s) to build.')
      .choices(KNOWN_ENVIRONMENTS)
      .default(KNOWN_ENVIRONMENTS),
  )
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { environment: environments } = options;

    const colors = [
      // enough colors, if more we repeat
      chalk.cyan,
      chalk.magenta,
      chalk.yellow,
      chalk.green,
      chalk.blue,
      chalk.white,
      chalk.gray,
    ];
    let colorIndex = 0;

    const procs: Promise<unknown>[] = [];
    const childProcs: ChildProcessWithoutNullStreams[] = [];
    const results: { id: string; success: boolean; duration: string }[] = [];
    const startTime = Date.now();

    ['SIGINT', 'SIGTERM'].forEach((signal) => {
      process.on(signal, () => {
        childProcs.forEach((cp) => cp.kill('SIGINT'));
        process.exit(1);
      });
    });

    for (const environment of environments) {
      const color = colors[colorIndex++ % colors.length];
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
        '--project-dir ../', // point to the outer dir because this is run in a nested folder
      ]
        .filter(Boolean)
        .join(' ');

      const proc = spawn(cmd, { shell: true });
      childProcs.push(proc);
      procs.push(
        new Promise((resolve) => {
          const procStart = Date.now();

          const finish = (code: number | null) => {
            const success = code === 0;
            const elapsed = ((Date.now() - procStart) / 1000).toFixed(1);
            results.push({ id, success, duration: elapsed });
            resolve(undefined);
          };

          if (isCI) {
            console.log(`::group::Build ${prefix}`);
            console.log(`${prefix} 🏗️ Running: ${cmd}`);
            // Buffer mode for CI
            let stdout = '';
            let stderr = '';

            proc.stdout.on('data', (data) => {
              stdout += data.toString();
            });
            proc.stderr.on('data', (data) => {
              stderr += data.toString();
            });

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
            rlOut.on('line', (line) => console.log(`${prefix} ${line}`));

            const rlErr = readline.createInterface({ input: proc.stderr });
            rlErr.on('line', (line) => console.error(`${prefix} ${line}`));

            proc.on('close', (code) => {
              finish(code);
            });
          }
        }),
      );
    }

    await Promise.all(procs);

    const duration = ((Date.now() - startTime) / 1000).toFixed(1);
    const succeeded = results.filter((r) => r.success);
    const failed = results.filter((r) => !r.success);
    console.log(`\n📊 Build Summary (${duration}s):`);
    succeeded.forEach((r) => console.log(`✅ ${r.id} (${r.duration}s)`));
    failed.forEach((r) => console.log(`❌ ${r.id} (${r.duration}s)`));

    if (failed.length > 0) {
      process.exit(1);
    } else {
      console.log(`\n🚀 All builds succeeded.`);
    }
  });

const flash = new Command('flash')
  .description('Flash firmware.')
  .addOption(
    new Option('--environment <environment>', 'The target to flash.').choices(KNOWN_ENVIRONMENTS).makeOptionMandatory(),
  )
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { environment } = options;
    console.log(`🚀 Flashing ${environment}`);

    const cmd = [
      'pio run',
      `--environment ${environment}`,
      '--target flash',
      '--project-dir ../', // point to the outer dir because this is run in a nested folder
    ]
      .filter(Boolean)
      .join(' ');
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
  .addOption(
    new Option('--environment [environment...]', 'The target(s) to build.')
      .choices(KNOWN_ENVIRONMENTS)
      .default(KNOWN_ENVIRONMENTS),
  )
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { environment: environments } = options;
    console.log(`📦 Collecting build artifacts ...`);
    const outputDir = 'binaries';
    if (existsSync(outputDir)) await rm(outputDir, { recursive: true });
    await mkdir(outputDir, { recursive: true });

    for (const environment of environments) {
      const buildDir = `.pio//build/${environment}`;
      const binSrc = join(buildDir, 'firmware.bin');
      const elfSrc = join(buildDir, 'firmware.elf');
      const binDest = join(outputDir, `${environment}.bin`);
      const elfDest = join(outputDir, `${environment}.elf`);

      console.log(`🔹 Copying ${binSrc} → ${binDest}`);
      await copyFile(binSrc, binDest);

      console.log(`🔹 Copying ${elfSrc} → ${elfDest}`);
      await copyFile(elfSrc, elfDest);
    }

    console.log(`✅ Done collecting binaries in ${outputDir}/`);
  });

function getAppVersionNumber(value: string) {
  // we need to generate the same value as APP_VERSION_NUMBER in version.h generated by version.py
  const version = semver.parse(value);
  if (!version) throw new Error(`Invalid version: ${value}`);

  // e.g. 0.4.0 → 0x000400 in hex which is 1024 in decimal. We need decimal version
  const v = (version.major << 16) | (version.minor << 8) | version.patch;
  return v;
}

const avail = new Command('avail')
  .description('Avail firmware to backend.')
  .addOption(new Option('--endpoint <endpoint>', 'The base URL to send requests.').default('http://localhost:3000'))
  .addOption(new Option('--key, --api-key <api-key>', 'The api key for authentication.').makeOptionMandatory())
  .addOption(new Option('--attestation <attestation>', 'Attestation URL').makeOptionMandatory())
  .addOption(new Option('--tag <tag>', 'The release tag.').makeOptionMandatory())
  .addOption(
    new Option('--environment [environment...]', 'The target(s) for which to avail.')
      .choices(KNOWN_ENVIRONMENTS)
      .default(KNOWN_ENVIRONMENTS),
  )
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { endpoint, apiKey, tag, attestation, environment: environments } = options;
    console.log(`📦 Availing firmware ...`);

    for (const environment of environments) {
      const fileName = `${environment}.bin`;

      // e.g. https://github.com/mburumaxwell/korra/releases/download/firmware-pio%400.4.0/arduino-keeper-esp32s3_devkitc.bin
      const url = `https://github.com/mburumaxwell/korra/releases/download/${encodeURIComponent(tag)}/${fileName}`;

      // tags are usually like firmware-pio@0.4.0
      // we need to extract the version from the tag
      const [, versionSemver] = tag.split('@');
      if (!versionSemver) {
        throw new Error(`Invalid tag format: ${tag}. Expected format: refs/tags/firmware-pio@<version>`);
      }
      const versionValue = getAppVersionNumber(versionSemver);

      const [framework, usage, board] = environment.split('-');
      const payload = {
        board,
        usage,
        framework,
        version: { value: versionValue, semver: versionSemver },
        url,
        attestation,
      };

      console.log(
        `🔹 Availing firmware for ${board} (${usage} built on ${framework}). Version: ${versionSemver} (${versionValue})`,
      );
      await fetch(`${endpoint}/api/github/firmware`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${apiKey}`,
        },
        body: JSON.stringify(payload),
      });
    }

    console.log(`✅ Done availing firmware to backend.`);
  });

const root = new Command();
root.name('korra-firmware').description('CLI too for running firmware tasks for Korra.');
root.usage();
root.version(packageJson.version, '--version');
root.addCommand(version);
root.addCommand(build);
root.addCommand(flash);
root.addCommand(collect);
root.addCommand(avail);

const args = process.argv;
root.parse(args);

// If no command is provided, show help
if (!args.slice(2).length) {
  root.help();
}
