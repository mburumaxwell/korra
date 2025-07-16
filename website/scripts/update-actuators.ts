import env from '@next/env';

import { ACTUATOR_TARGETS_POT } from '@/lib/actuator';
import { getRegistry, getTwin } from '@/lib/iot-hub';
import { type KorraActuatorConfig, type KorraUsageType } from '@/lib/schemas';

type RunProps = {
  /**
   * The firmware framework to update devices for.
   * @default 'arduino'
   */
  usage?: KorraUsageType;

  /**
   * Whether to enable the actuator.
   * @default true
   */
  enabled?: boolean;

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
 * This script is used to update desired actuator values by updating the device twin's desired properties.
 *
 * It assumes the firmware is not buggy.
 */
async function run(props: RunProps) {
  env.loadEnvConfig(process.cwd());

  const { prisma } = await import('@/lib/prisma');

  // fetch devices
  const { usage, enabled = true, count } = props;
  const devices = await prisma.device.findMany({ where: { usage }, include: { actuator: true } });

  // work on each device
  let updated = 0;
  const registry = getRegistry();
  for (const device of devices) {
    // if a specific device ID is provided, skip others
    const { id, label } = device;
    if (props.devices && !props.devices.includes(id)) {
      console.log(`Skipping device ${label} (${id}) as it is not in the specified devices list.`);
      continue;
    }

    // if a specific device ID is excluded, skip it
    if (props.exclude && props.exclude.includes(id)) {
      console.log(`Skipping device ${label} (${id}) as it is in the excluded devices list.`);
      continue;
    }

    // skip devices that have not reported their actuator state (haven't come online yet)
    console.log(`Fetching twin for device ${label} (${id})`);
    const twin = await getTwin(registry, id);
    const reported = twin.properties.reported.actuator;
    if (!reported) {
      console.log(`device ${label} (${id}) has not reported its actuator state yet. Skipping.`);
      continue;
    }

    // keepers get longer duration because it is just a fan
    // pots get shorter duration because it is a pump
    const duration = usage === 'keeper' ? 30 : 5; // seconds
    const equilibrium_time = usage === 'keeper' ? 15 : 5; // seconds
    const target = usage === 'keeper' ? 36 : ACTUATOR_TARGETS_POT[device.moistureCategory!];
    const config: KorraActuatorConfig = {
      enabled,
      duration,
      equilibrium_time,
      target,
    };

    console.log(`Updating actuator for device ${label} (${id}), ${JSON.stringify(config)}`);
    await registry.updateTwin(id, { properties: { desired: { actuator: config } } }, twin.etag);
    updated++;

    if (count && updated >= count) {
      console.log(`Updated ${updated} devices, stopping as per count limit.`);
      break;
    }
  }
}

await run({ usage: 'keeper' });
// await run({ usage: 'keeper', enabled: false });
// await run({ usage: 'pot' });
// await run({ usage: 'pot', count: 2, exclude: ['ac9273f93030'] });
// await run({ usage: 'pot', devices: ['ac9273f93030'] });
