#!/usr/bin/env node

import { Command, Option } from 'commander';
import crypto from 'node:crypto';
import { existsSync } from 'node:fs';
import { copyFile, mkdir, readFile, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import process from 'node:process';
import * as semver from 'semver';

import packageJson from '../package.json' with { type: 'json' };

const KNOWN_ENVIRONMENTS = [
  'arduino-keeper-esp32c6_devkitc',
  'arduino-pot-esp32c6_devkitc',
  'arduino-keeper-esp32s3_devkitc',
  'arduino-pot-esp32s3_devkitc',

  // 'espidf-keeper-esp32c6_devkitc',
  // 'espidf-pot-esp32c6_devkitc',
  // 'espidf-keeper-esp32s3_devkitc',
  // 'espidf-pot-esp32s3_devkitc',
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

    // ensure the output directory exists, this is where the collected binaries are
    const outputDir = 'binaries';
    if (!existsSync(outputDir)) {
      throw new Error(`Output directory '${outputDir}' does not exist. Please run 'collect' first.`);
    }

    // tags are usually like firmware-pio@0.4.0
    // we need to extract the version from the tag
    const [, versionSemver] = tag.split('@');
    if (!versionSemver) {
      throw new Error(`Invalid tag format: ${tag}. Expected format: refs/tags/firmware-pio@<version>`);
    }
    const versionValue = getAppVersionNumber(versionSemver);

    for (const environment of environments) {
      const fileName = `${environment}.bin`;
      const filePath = join(outputDir, fileName);
      if (!existsSync(filePath)) {
        throw new Error(`Missing expected file: ${filePath}`);
      }

      // compute hash
      const buffer = await readFile(filePath);
      const hash = crypto.createHash('sha256').update(buffer).digest('hex');

      // e.g. https://github.com/mburumaxwell/korra/releases/download/firmware-pio%400.4.0/arduino-keeper-esp32s3_devkitc.bin
      const url = `https://github.com/mburumaxwell/korra/releases/download/${encodeURIComponent(tag)}/${fileName}`;

      const [framework, usage, board] = environment.split('-');
      const payload = {
        board,
        usage,
        framework,
        version: { value: versionValue, semver: versionSemver },
        url,
        attestation,
        hash,
        signature: 'tbd', // TODO: pull signature from attestation URL
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
root.addCommand(collect);
root.addCommand(avail);

const args = process.argv;
root.parse(args);

// If no command is provided, show help
if (!args.slice(2).length) {
  root.help();
}
