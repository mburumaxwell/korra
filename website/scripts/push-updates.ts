import dotenv from 'dotenv-flow';

import { getRegistry, getTwin } from '../src/lib/iot-hub.ts';
import {
  KorraBoardTypes,
  type KorraDeviceTwinDesiredFirmware,
  type KorraFirmwareFramework,
  KorraUsageTypes,
} from '../src/lib/schemas.ts';

type RunProps = {
  /**
   * The firmware framework to update devices for.
   * @default 'arduino'
   */
  framework?: KorraFirmwareFramework;

  /**
   * The version of the firmware to update devices to.
   * If not specified, the latest version will be used.
   */
  version?: string;

  /**
   * The number of devices to update.
   * If not specified, all devices of the specified framework will be updated.
   */
  count?: number;

  /**
   * List of device IDs to update.
   * If not specified, all devices of the specified framework will be updated or according to the `count` parameter.
   */
  devices?: string[];

  /**
   * List of device IDs to exclude from the update.
   * If specified, these devices will not be updated even if they match the framework and version.
   */
  exclude?: string[];
};

/**
 * This script is used to push updates to the devices in the registry by reading the latest firmware from the database
 * and updating the device twin's desired properties.
 *
 * It assumes the firmware is not buggy and pushes it to all devices of the specified framework.
 */
async function run(props: RunProps) {
  dotenv.config();
  const { prisma } = await import('../src/lib/prisma/index.ts');

  // fetch the latest available firmware for the specified framework
  // the latest firmware for a framework would be multiple entries, combinations: board * usage
  const combinations = KorraUsageTypes.length * KorraBoardTypes.length;
  const { framework = 'arduino', version: targetVersionSemver, count } = props;
  let firmwareEntries = await prisma.availableFirmware.findMany({
    where: { framework, ...(targetVersionSemver && { versionSemver: targetVersionSemver }) },
    orderBy: { created: 'desc' },
    take: combinations,
  });

  if (!targetVersionSemver) {
    // find the latest version and only keep entries from it
    const latestVersion = firmwareEntries.map((f) => f.versionValue).sort((a, b) => b - a)[0];
    firmwareEntries = firmwareEntries.filter((f) => f.versionValue === latestVersion);
  }

  // ensure we have at least one firmware available
  if (firmwareEntries.length === 0) {
    throw new Error(`No firmware available for framework: ${framework}`);
  }

  const resolvedVersionSemver = firmwareEntries[0].versionSemver;
  const resolvedVersionValue = firmwareEntries[0].versionValue;
  console.log(
    `Updating devices running '${framework}' to ${resolvedVersionSemver} (${resolvedVersionValue} or 0x${resolvedVersionValue.toString(16).padStart(5, '0')})`,
  );

  // fetch devices
  const devices = await prisma.device.findMany({ where: { framework }, include: { firmware: true } });

  // work on each device
  let updated = 0;
  const registry = getRegistry();
  for (const device of devices) {
    // if a specific device ID is provided, skip others
    const id = device.id;
    if (props.devices && !props.devices.includes(id)) {
      console.log(`Skipping device ${id} as it is not in the specified devices list.`);
      continue;
    }

    // if a specific device ID is excluded, skip it
    if (props.exclude && props.exclude.includes(id)) {
      console.log(`Skipping device ${id} as it is in the excluded devices list.`);
      continue;
    }

    // find the target firmware for the device
    const targetFirmware = firmwareEntries.find(
      (f) => f.board === device.board && f.usage === device.usage && f.framework === device.framework,
    );
    if (!targetFirmware) {
      console.warn(
        `No matching firmware found for device ${id} with board ${device.board}, usage ${device.usage}, framework ${device.framework}`,
      );
      continue;
    }

    // skip devices that have not reported their firmware version (haven't come online yet)
    console.log(`Fetching twin for device ${id}`);
    const twin = await getTwin(registry, id);
    const reported = twin.properties.reported.firmware?.version;
    if (!reported?.semver || !reported?.value) {
      console.warn(`Device ${id} has no reported firmware version, skipping update.`);
      continue;
    }

    // if the device is not in the specified devices list, check if it needs an update
    // if it is in the list, we will update it regardless of its current version
    if (!props.devices || !props.devices.includes(id)) {
      // skip devices that are already at or above the target version
      if (reported.value >= resolvedVersionValue) {
        console.log(
          `Device ${id} is already at or above version ${reported.semver} (${reported.value}), skipping update.`,
        );
        continue;
      }

      // skip devices that are already at the desired version
      const desired = twin.properties.desired.firmware?.version;
      if (desired?.semver === resolvedVersionSemver && desired.value === resolvedVersionValue) {
        console.log(`Device ${id} is already at version ${desired?.semver} (${desired?.value}), skipping update.`);
        continue;
      }
    }

    console.log(`Updating device ${id} to version ${resolvedVersionSemver} (${resolvedVersionValue})`);
    await registry.updateTwin(
      id,
      {
        properties: {
          desired: {
            firmware: {
              version: {
                semver: resolvedVersionSemver,
                value: resolvedVersionValue,
              },
              url: targetFirmware.url,
              hash: targetFirmware.hash,
              signature: targetFirmware.signature,
            } satisfies KorraDeviceTwinDesiredFirmware,
          },
        },
      },
      twin.etag,
    );
    updated++;

    if (count && updated >= count) {
      console.log(`Updated ${updated} devices, stopping as per count limit.`);
      break;
    }
  }
}

await run({ version: '0.4.0', count: 1 });
// await run({ version: '0.4.0', count: 2, exclude: ['ac9273f93030'] });
// await run({ version: '0.4.0', devices: ['ac9273f93030'] });
