#!/usr/bin/env node

import chalk from 'chalk';
import { Command, Option } from 'commander';
import { execSync, spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import { copyFile, mkdir, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import process from 'node:process';
import readline from 'node:readline';
import * as semver from 'semver';

import packageJson from '../package.json' with { type: 'json' };

// There might be lots of overlap in this file/tool with west or scripts in a west manifest.
// However, this is more readable/maintainable than Python scripts, has things like parallelism and versioning.
// It restricts the apps/boards to what is actually supported by the firmware.

const isCI = Boolean(process.env['CI'] || process.env['GITHUB_ACTIONS']);
const KNOWN_APPS = ['keeper', 'pot'];
const KNOWN_BOARDS = [
  'esp32s3_devkitc/esp32s3/procpu',
  'frdm_rw612',
  'nrf7002dk/nrf5340/cpuapp',
  // add other board targets here
];
const KNOWN_BLOBS_MODULES = [
  'hal_espressif',
  'hal_nxp',
  'nrf_wifi',
  // add other blobs needed here
];

const makeBuildDir = ({ app, board }) => `build/${app}/${board}`;

const version = new Command('version')
  .description('Generate VERSION file for zephyr.')
  .option('--dev', 'Whether in dev mode.')
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { dev } = options;

    let version = semver.parse(packageJson.version);
    if (!version) {
      throw new Error('Invalid version in package.json');
    }

    const validateByte = (num, label) =>
      (num >= 0 && num <= 255) ||
      (() => {
        throw new Error(`${label} must be in range 0-255`);
      })();

    let tweak = version.build.length > 0 ? parseInt(version.build[0], 10) : 0;
    let extraversion =
      version.prerelease.length > 0
        ? version.prerelease
            .join('.')
            .toLowerCase()
            .replace(/[^a-z0-9.-]/g, '')
        : '';

    if (dev) {
      // Mark tweak as 255 (reserved for dev)
      tweak = 255;
      extraversion = extraversion ? `${extraversion}.dev` : 'dev';
    }

    validateByte(version.major, 'Major');
    validateByte(version.minor, 'Minor');
    validateByte(version.patch, 'Patch');
    validateByte(tweak, 'Tweak');

    // https://docs.zephyrproject.org/latest/build/version/index.html
    const lines = [
      `VERSION_MAJOR = ${version.major}`,
      `VERSION_MINOR = ${version.minor}`,
      `PATCHLEVEL = ${version.patch}`,
      `VERSION_TWEAK = ${tweak}`,
      `EXTRAVERSION = ${extraversion}`,
      '', // ensure newline
    ];

    const contents = lines.join('\n');
    const destination = join(process.cwd(), './VERSION');
    if (existsSync(destination)) await rm(destination);
    await writeFile(destination, contents, 'utf-8');
    console.log(`✅ Generated VERSION file for ${version} at ${destination}`);
  });

const build = new Command('build')
  .description('Build firmware.')
  .addOption(new Option('--app [app...]', 'The app kind(s).').choices(KNOWN_APPS).default(KNOWN_APPS))
  .addOption(
    new Option('--board [board...]', 'Board(s) to build for with optional board revision.')
      .choices(KNOWN_BOARDS)
      .default(KNOWN_BOARDS),
  )
  .option('-p, --pristine', 'Whether to build pristine (from scratch).')
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { app: apps, board: boards, pristine } = options;

    // if we are not in CI and the local file exists, mark it for inclusion
    const local = !isCI && existsSync('local.conf');
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

    const procs = [];
    const childProcs = [];
    const results = []; // NEW: keep success/failure per task
    const startTime = Date.now(); // NEW: track elapsed time

    ['SIGINT', 'SIGTERM'].forEach((signal) => {
      process.on(signal, () => {
        childProcs.forEach((cp) => cp.kill('SIGINT'));
        process.exit(1);
      });
    });

    for (const app of apps) {
      for (const board of boards) {
        const buildDir = makeBuildDir({ app, board });
        const color = colors[colorIndex++ % colors.length];
        const id = `[${app}@${board}]`;
        const prefix = !isCI ? color(id) : id;

        // The --pristine/-p option takes: auto|always|never
        // Passing --pristine/-p is equivalent to passing 'always' value.
        // Docs: https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html#pristine-builds
        // If we include any --extra* option, we force pristine but mostly in dev, we need to save time
        // The --extra* options are only passed if we have pristine which means by choice
        // Scenarios where passing --pristine makes sense is in CI where the extra files are not already known
        // or in dev when we add a new file or feel there is a need for it
        const cmd = [
          'west build',
          `--board ${board}`,
          `--build-dir ${buildDir}`,
          pristine && '--pristine',
          pristine && `--extra-dtc-overlay apps/${app}.overlay`,
          pristine && `--extra-conf apps/${app}.conf`,
          pristine && local && '--extra-conf local.conf',
        ]
          .filter(Boolean)
          .join(' ');

        const proc = spawn(cmd, { shell: true });
        childProcs.push(proc);
        procs.push(
          new Promise((resolve) => {
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
  .addOption(new Option('--app <app>', 'The app kind(s).').choices(KNOWN_APPS).makeOptionMandatory())
  .addOption(
    new Option('--board <board>', 'Board(s) to build for with optional board revision.')
      .choices(KNOWN_BOARDS)
      .makeOptionMandatory(),
  )
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { app, board } = options;
    const buildDir = makeBuildDir({ app, board });
    console.log(`🚀 Flashing app ${app} to ${board}`);

    const cmd = ['west flash', `--build-dir ${buildDir}`].filter(Boolean).join(' ');
    console.log(`🏗️ Running: ${cmd}`);

    try {
      execSync(cmd, { stdio: 'inherit' });
      console.log(`✅ Flashing ${app} to ${board} completed`);
    } catch (error) {
      console.error(`❌ Flash failed:`, error.message);
      process.exit(1);
    }
  });

const collect = new Command('collect')
  .description('Collect firmware binaries.')
  .addOption(new Option('--app [app...]', 'The app kind(s).').choices(KNOWN_APPS).default(KNOWN_APPS))
  .addOption(
    new Option('--board [board...]', 'Board(s) to collect for with optional board revision.')
      .choices(KNOWN_BOARDS)
      .default(KNOWN_BOARDS),
  )
  .action(async (...args) => {
    const [options /*, command*/] = args;
    const { app: apps, board: boards } = options;
    console.log(`📦 Collecting build artifacts ...`);
    const outputDir = 'binaries';
    if (existsSync(outputDir)) await rm(outputDir, { recursive: true });
    await mkdir(outputDir, { recursive: true });

    for (const app of apps) {
      for (const board of boards) {
        const buildDir = `build/${app}/${board}/zephyr`;
        const binSrc = join(buildDir, 'zephyr.bin');
        const elfSrc = join(buildDir, 'zephyr.elf');
        const binDest = join(outputDir, `${app}-${board.split('/')[0]}.bin`);
        const elfDest = join(outputDir, `${app}-${board.split('/')[0]}.elf`);

        console.log(`🔹 Copying ${binSrc} → ${binDest}`);
        await copyFile(binSrc, binDest);

        console.log(`🔹 Copying ${elfSrc} → ${elfDest}`);
        await copyFile(elfSrc, elfDest);
      }
    }

    console.log(`✅ Done collecting binaries in ${outputDir}/`);
  });

const blobs = new Command('blobs')
  .description('Manage blobs')
  .addCommand(
    new Command('fetch')
      .description('Fetch blobs')
      .addOption(
        new Option('--module [module...]', 'The blob module.')
          .choices(KNOWN_BLOBS_MODULES)
          .default(KNOWN_BLOBS_MODULES),
      )
      .addOption(
        new Option('-a, --auto-accept [module]', 'auto accept license if the fetching needs click-through').default(
          true,
        ),
      )
      .action(async (...args) => {
        const [options /*, command*/] = args;
        const { module: modules, autoAccept } = options;

        for (const module of modules) {
          const cmd = ['west blobs fetch', module, autoAccept && '--auto-accept'].filter(Boolean).join(' ');
          console.log(`🏗️ Running: ${cmd}`);

          try {
            execSync(cmd, { stdio: 'inherit' });
            // console.log(`✅ Blobs fetched`);
          } catch (error) {
            console.error(`❌ Blobs fetch failed:`, error.message);
            process.exit(1);
          }
        }
      }),
  )
  .addCommand(
    new Command('clean').description('Clean blobs').action(async (...args) => {
      const cmd = ['west blobs clean'].filter(Boolean).join(' ');
      console.log(`🏗️ Running: ${cmd}`);

      try {
        execSync(cmd, { stdio: 'inherit' });
        console.log(`✅ Blobs cleaned`);
      } catch (error) {
        console.error(`❌ Blobs clean failed:`, error.message);
        process.exit(1);
      }
    }),
  );

const root = new Command();
root.name('korra-firmware').description('CLI too for running firmware tasks for Korra.');
root.usage();
root.version(packageJson.version, '--version');
root.addCommand(version);
root.addCommand(build);
root.addCommand(flash);
root.addCommand(collect);
root.addCommand(blobs);

const args = process.argv;
root.parse(args);

// If no command is provided, show help
if (!args.slice(2).length) {
  root.help();
}
