/* eslint-disable no-undef */

import { existsSync } from 'node:fs';
import { readdir, readFile, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import * as semver from 'semver';

function validateByte(num, label) {
  // Zephyr requires 0–255 range
  if (num < 0 || num > 255) {
    throw new Error(`${label} must be in range 0-255`);
  }
}

async function generate({ dev, dir }) {
  const packageJson = JSON.parse(await readFile(join(dir, 'package.json'), 'utf-8'));
  let version = semver.parse(packageJson.version);
  if (!version) {
    throw new Error('Invalid version in package.json');
  }

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
  const destination = join(dir, './VERSION')
  if (existsSync(destination)) await rm(destination);
  await writeFile(destination, contents, 'utf-8');
  console.log(`✅ Generated VERSION file for ${version} at ${destination}`);
}

async function run(dev) {
  const firmwareDir = join(process.cwd(), 'firmware'); // resolve firmware directory from project root
  const entries = await readdir(firmwareDir, { withFileTypes: true });
  const folders = entries.filter(entry => entry.isDirectory()).map(entry => join(firmwareDir, entry.name));

  for(const dir of folders) {
    generate({ dev, dir, });
  }
}

await run(process.argv.includes('--dev'));
