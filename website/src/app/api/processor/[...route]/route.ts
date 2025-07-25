import { zValidator } from '@hono/zod-validator';
import { Hono } from 'hono';
import { bearerAuth } from 'hono/bearer-auth';
import { handle } from 'hono/vercel';

import { getRegistry, getTwin } from '@/lib/iot-hub';
import { prisma } from '@/lib/prisma';
import {
  OperationalEventRequestBodySchema,
  TelemetryRequestBodyActuatorsSchema,
  TelemetryRequestBodySensorsSchema,
} from '@/lib/schemas';

export const dynamic = 'force-dynamic';

const app = new Hono().basePath('/api/processor');
app.use('/*', bearerAuth({ token: `${process.env.PROCESSOR_API_KEY}` }));

app.post('/telemetry/sensors', zValidator('json', TelemetryRequestBodySensorsSchema), async (context) => {
  const telemetry = context.req.valid('json');

  const { id, device_id: deviceId, app_kind: kind } = telemetry;
  const device = await prisma.device.findUnique({ where: { id: deviceId } });
  if (!device) {
    // technically this should not happen, but it might for existing test devices
    // we log then fail silently
    console.warn(`Device with ID ${deviceId} not found. Telemetry will not be processed.`);
    return context.body(null, 204);
  }

  // if we receive telemetry for a device that is not of the expected kind, we ignore it
  if (kind !== device.usage) {
    console.warn(
      `Received telemetry for device ${deviceId} of kind ${kind}, but expected ${device.usage}. Ignoring telemetry.`,
    );
    return context.body(null, 204);
  }

  // store telemetry in the database, if it does not already exist
  const values = {
    usage: kind,
    created: telemetry.created,
    received: telemetry.received ?? new Date(),
    temperature: kind === 'keeper' ? telemetry.temperature : undefined,
    humidity: kind === 'keeper' ? telemetry.humidity : undefined,
    moisture: kind === 'pot' ? telemetry.moisture : undefined,
    ph: kind === 'pot' ? telemetry.ph : undefined,
  };
  await prisma.deviceTelemetrySensors.upsert({
    where: { id: id },
    create: { id, deviceId, ...values },
    update: { deviceId, ...values },
  });

  // set last seen if not present or telemetry is newer
  if (!device.lastSeen || telemetry.created.getTime() > device.lastSeen?.getTime()) {
    await prisma.device.update({
      where: { id: deviceId },
      data: { lastSeen: telemetry.created },
    });
  }

  return context.body(null, 201);
});

app.post('/telemetry/actuators', zValidator('json', TelemetryRequestBodyActuatorsSchema), async (context) => {
  const telemetry = context.req.valid('json');

  const { id, device_id: deviceId, app_kind: kind } = telemetry;
  const device = await prisma.device.findUnique({ where: { id: deviceId } });
  if (!device) {
    // technically this should not happen, but it might for existing test devices
    // we log then fail silently
    console.warn(`Device with ID ${deviceId} not found. Telemetry will not be processed.`);
    return context.body(null, 204);
  }

  // if we receive telemetry for a device that is not of the expected kind, we ignore it
  if (kind !== device.usage) {
    console.warn(
      `Received telemetry for device ${deviceId} of kind ${kind}, but expected ${device.usage}. Ignoring telemetry.`,
    );
    return context.body(null, 204);
  }

  // store telemetry in the database, if it does not already exist
  const values = {
    usage: kind,
    created: telemetry.created,
    received: telemetry.received ?? new Date(),
    fan: kind === 'keeper' ? telemetry.fan.duration : undefined,
    pump: kind === 'pot' ? telemetry.pump.duration : undefined,
  };
  await prisma.deviceTelemetryActuation.upsert({
    where: { id: id },
    create: { id, deviceId, ...values },
    update: { deviceId, ...values },
  });

  // set last seen if not present or telemetry is newer
  if (!device.lastSeen || telemetry.created.getTime() > device.lastSeen?.getTime()) {
    await prisma.device.update({
      where: { id: deviceId },
      data: { lastSeen: telemetry.created },
    });
  }

  return context.body(null, 201);
});

app.post('/operational-event', zValidator('json', OperationalEventRequestBodySchema), async (context) => {
  const event = context.req.valid('json');
  console.log('Received operational event', event);

  const { device_id: deviceId, type, received } = event;
  const device = await prisma.device.findUnique({ where: { id: deviceId } });
  if (!device) {
    // technically this should not happen, but it might for existing test devices
    // we log then fail silently
    console.warn(`Device with ID ${deviceId} not found. Operational event will not be processed.`);
    return context.body(null, 204);
  }

  const registry = getRegistry();

  if (type === 'connected') {
    if (!device.connected) {
      await prisma.device.update({
        where: { id: deviceId },
        data: { connected: true, lastSeen: moreRecent(device.lastSeen, received) },
      });
    }
  } else if (type === 'disconnected') {
    if (device.connected) {
      await prisma.device.update({
        where: { id: deviceId },
        data: { connected: false },
      });
    }
  } else if (type === 'twin.updated') {
    // for twin updates, we pull the device twin and update the database
    const twin = await getTwin(registry, deviceId);
    if (twin) {
      const reported = twin.properties.reported;
      // checking undefined not null because null is used to unset
      if (reported.firmware !== undefined) {
        await prisma.deviceFirmware.upsert({
          where: { deviceId },
          create: {
            deviceId,
            currentVersion: reported.firmware?.version?.semver,
            desiredVersion: null,
            desiredFirmwareId: null,
          },
          update: {
            currentVersion: reported.firmware?.version?.semver,
          },
        });
      }

      // checking undefined not null because null is used to unset
      if (reported.network !== undefined) {
        await prisma.deviceNetwork.upsert({
          where: { deviceId },
          create: {
            deviceId,
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
        });
      }

      // checking undefined not null because null is used to unset
      if (reported.actuator !== undefined) {
        const lastTime = reported.actuator?.last_time ? new Date(reported.actuator?.last_time) : null;
        await prisma.deviceActuator.upsert({
          where: { deviceId },
          create: {
            deviceId,

            enabled: false,
            duration: null,
            equilibriumTime: null,
            target: null,

            count: reported.actuator?.count ?? 0,
            lastTime,
            totalDuration: reported.actuator?.total_duration,
          },
          update: {
            count: reported.actuator?.count ?? 0,
            lastTime,
            totalDuration: reported.actuator?.total_duration,
          },
        });
      }
    }
  }

  return context.body(null, 201);
});

function moreRecent(first?: Date | null, second?: Date | null) {
  if (!first || !second) return second || first;
  return first.getTime() > second.getTime() ? first : second;
}

export const OPTIONS = handle(app);
export const GET = handle(app);
export const POST = handle(app);
export const PUT = handle(app);
export const PATCH = handle(app);
export const DELETE = handle(app);
