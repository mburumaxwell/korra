#!/usr/bin/env node

import { Argument, Command } from 'commander';
import { existsSync } from 'node:fs';
import { mkdir, copyFile, rm, writeFile } from 'node:fs/promises';
import { execSync, spawn, spawnSync } from 'node:child_process';
import { join } from 'node:path';
import process from 'node:process';
import * as semver from 'semver';

import packageJson from '../../package.json' with { type: "json" };

const isCI = Boolean(process.env['CI'] || process.env['GITHUB_ACTIONS']);
const KNOWN_APPS = ['keeper', 'pot'];
const KNOWN_BOARDS = [
  'esp32s3_devkitc/esp32s3/procpu',
  'frdm_rw612',
];

const makeBuildDir = ({ app, board }) => `build/${app}/${board}`;
const appArgument = new Argument('<app>', 'The app kind.').choices(KNOWN_APPS);
const boardArgument = new Argument('<board>', 'Board to build for with optional board revision.').choices(KNOWN_BOARDS);

const version = new Command('version')
  .description('Generate VERSION file for zephyr.')
  .option('--dev', 'Whether in dev mode.')
  .action(async (...args) => {
    const [options/*, command*/] = args;
    const { dev } = options;

    let version = semver.parse(packageJson.version);
    if (!version) {
      throw new Error('Invalid version in package.json');
    }

    const validateByte = (num, label) => (num >= 0 && num <= 255) || (() => { throw new Error(`${label} must be in range 0-255`); })();

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
    const destination = join(process.cwd(), './VERSION')
    if (existsSync(destination)) await rm(destination);
    await writeFile(destination, contents, 'utf-8');
    console.log(`✅ Generated VERSION file for ${version} at ${destination}`);
  });

const build = new Command('build')
  .description('Build firmware.')
  .addArgument(appArgument)
  .addArgument(boardArgument)
  .option('--pristine', 'Whether to build pristine (from scratch).')
  .action(async (...args) => {
    const [app, board, options/*, command*/] = args;
    const buildDir = makeBuildDir({ app, board });
    if (isCI) console.log(`::group::Building app: ${app} for: ${board}`);
    console.log(`🚀 Building app: ${app}, targeting: ${board}`);
    const { pristine } = options;

    // if we are not in CI and the local file exists, mark it for inclusion
    const local = !isCI && existsSync('local.conf');

    const cmd = [
      'west build',
      `--board ${board}`,
      `--build-dir ${buildDir}`,
      (pristine && '--pristine'),
      `--extra-dtc-overlay apps/${app}.overlay`,
      `--extra-conf apps/${app}.conf`,
      (local && '--extra-conf local.conf'),
    ].filter(Boolean).join(' ');
    console.log(`🏗️ Running: ${cmd}`);

    const result = spawnSync(cmd, { shell: true, stdio: 'inherit' });
    console.log(`✅ Build completed for app: ${app}, targeting: ${board}`);

    if (isCI) console.log(`::endgroup::`);

    if (result.status !== 0) {
      process.exit(result.status);
    }
  });

const flash = new Command('flash')
  .description('Flash firmware.')
  .addArgument(appArgument)
  .addArgument(boardArgument)
  .action(async (...args) => {
    const [app, board, options/*, command*/] = args;
    const buildDir = makeBuildDir({ app, board });
    console.log(`🚀 Flashing app ${app} to ${board}`);
    const { } = options;

    const cmd = [
      'west flash',
      `--build-dir ${buildDir}`,
    ].filter(Boolean).join(' ');
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
  .action(async () => {
    console.log(`📦 Collecting build artifacts ...`);
    const outputDir = 'binaries';
    await mkdir(outputDir, { recursive: true });

    for (const app of KNOWN_APPS) {
      for (const board of KNOWN_BOARDS) {
        const buildDir = `build/${app}/${board}/zephyr`;
        const binSrc = join(buildDir, 'zephyr.bin');
        const elfSrc = join(buildDir, 'zephyr.elf');
        const binDest = join(outputDir, `${app}-${board.replaceAll('/', '_')}.bin`);
        const elfDest = join(outputDir, `${app}-${board.replaceAll('/', '_')}.elf`);

        console.log(`🔹 Copying ${binSrc} → ${binDest}`);
        await copyFile(binSrc, binDest);

        console.log(`🔹 Copying ${elfSrc} → ${elfDest}`);
        await copyFile(elfSrc, elfDest);
      }
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
