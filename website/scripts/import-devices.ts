import dotenv from 'dotenv-flow';
import { readFile } from 'node:fs/promises';

import { getRegistry, getTwin } from '@/lib/iot-hub';
import { type AvailableFirmware, type MoistureCategory } from '@/lib/prisma/client';
import { type KorraBoardType, type KorraFirmwareFramework } from '@/lib/schemas';

async function run() {
  dotenv.config();
  const { prisma } = await import('@/lib/prisma');
  const registry = getRegistry();
  const devices = (await registry.list()).responseBody;
  console.log(`Found ${devices.length} devices in the IoT Hub.`);

  for (const device of devices) {
    const { deviceId } = device;
    console.log(`Processing device: ${deviceId}`);
    const twin = await getTwin(registry, deviceId);
    const { tags } = twin;
    const { usage, label } = tags;

    if (!usage || !label) {
      console.warn(`Device ${deviceId} is missing usage or label tags. Skipping.`);
      continue;
    }

    // Read the certificate PEM file based on the label
    const certificatePem = await readFile(`scripts/provisioning/certs/${label?.toLowerCase()}.cer`, 'utf-8');

    // if the lastActivityTime is more than one month ago, set to null
    let lastSeen = device.lastActivityTime ? new Date(device.lastActivityTime) : null;
    lastSeen = lastSeen && Date.now() - lastSeen.getTime() < 30 * 24 * 60 * 60 * 1000 ? lastSeen : null;

    const { desired, reported } = twin.properties;
    const lastTime = reported.actuator?.last_time ? new Date(reported.actuator?.last_time) : null;

    const board: KorraBoardType = 'esp32s3_devkitc'; // the ones we deployed are all ESP32-S3 DevKitC
    const framework: KorraFirmwareFramework = 'arduino'; // the ones we deployed are all Arduino

    let desiredFirmware: AvailableFirmware | undefined;
    if (desired.firmware?.version?.semver) {
      desiredFirmware = await prisma.availableFirmware.findUniqueOrThrow({
        where: {
          board_usage_framework_versionSemver: {
            board,
            usage,
            framework,
            versionSemver: desired.firmware?.version?.semver,
          },
        },
      });

      if (!desiredFirmware) {
        console.warn('Unable to find matching firmware row');
      }
    }

    // compute moisture category for pots based on the label
    const lfc = label[0].toUpperCase();
    const moistureCategory: MoistureCategory | null =
      lfc === 'D' ? 'dry' : lfc === 'M' ? 'medium' : lfc === 'W' ? 'wet' : null;

    const connected = device.connectionState?.toLocaleLowerCase() === 'connected';
    await prisma.device.upsert({
      where: { id: deviceId },
      create: {
        id: deviceId,
        created: new Date('2025-06-30T13:00:00Z'), // all devices were created around same time

        board,
        usage,
        framework,
        label,
        moistureCategory,
        certificatePem,
        connected,
        lastSeen: device.lastActivityTime ? new Date(device.lastActivityTime) : null,
        firmware: {
          create: {
            currentVersion: reported.firmware?.version?.semver,
            desiredVersion: desiredFirmware?.versionSemver,
            desiredFirmwareId: desiredFirmware?.id,
          },
        },
        actuator: {
          create: {
            enabled: desired.actuator?.enabled,
            duration: desired.actuator?.duration,
            equilibriumTime: desired.actuator?.equilibrium_time,
            target: desired.actuator?.target,

            count: reported.actuator?.count ?? 0,
            lastTime,
            totalDuration: reported.actuator?.total_duration,
          },
        },
        network: {
          create: {
            kind: reported.network?.kind,
            name: reported.network?.name,
            mac: reported.network?.mac,
            localIp: reported.network?.local_ip,
          },
        },
      },
      update: {
        board,
        usage,
        framework,
        label,
        moistureCategory,
        certificatePem,
        connected,
        lastSeen: device.lastActivityTime ? new Date(device.lastActivityTime) : null,
        firmware: {
          upsert: {
            create: {
              currentVersion: reported.firmware?.version?.semver,
              desiredVersion: desiredFirmware?.versionSemver,
              desiredFirmwareId: desiredFirmware?.id,
            },
            update: {
              currentVersion: reported.firmware?.version?.semver,
              desiredVersion: desiredFirmware?.versionSemver,
              desiredFirmwareId: desiredFirmware?.id,
            },
          },
        },
        actuator: {
          upsert: {
            create: {
              enabled: desired.actuator?.enabled,
              duration: desired.actuator?.duration,
              equilibriumTime: desired.actuator?.equilibrium_time,
              target: desired.actuator?.target,

              count: reported.actuator?.count ?? 0,
              lastTime,
              totalDuration: reported.actuator?.total_duration,
            },
            update: {
              enabled: desired.actuator?.enabled,
              duration: desired.actuator?.duration,
              equilibriumTime: desired.actuator?.equilibrium_time,
              target: desired.actuator?.target,

              count: reported.actuator?.count ?? 0,
              lastTime,
              totalDuration: reported.actuator?.total_duration,
            },
          },
        },
        network: {
          upsert: {
            create: {
              kind: reported.network?.kind,
              name: reported.network?.name,
              mac: reported.network?.mac,
              localIp: reported.network?.local_ip,
            },
            update: {
              kind: reported.network?.kind,
              name: reported.network?.name,
              mac: reported.network?.mac,
              localIp: reported.network?.local_ip,
            },
          },
        },
      },
    });
  }
}

await run();
