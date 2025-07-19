import { Hono } from 'hono';
import { bearerAuth } from 'hono/bearer-auth';
import { handle } from 'hono/vercel';

import { prisma } from '@/lib/prisma';
import { type DeviceTelemetryActuation, type DeviceTelemetrySensors } from '@/lib/prisma/client';

export const dynamic = 'force-dynamic';

const app = new Hono().basePath('/api/devices');
app.use('/*', bearerAuth({ token: `${process.env.PROCESSOR_API_KEY}` }));

app.get('/', async (context) => {
  const devices = await prisma.device.findMany({ include: { firmware: true, network: true } });
  return context.json(devices.map((d) => ({ ...d, certificatePem: undefined })));
});

app.get('/:id', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id }, include: { firmware: true, network: true } });
  return context.json(device);
});

function simplifyTelemetrySensors(value: DeviceTelemetrySensors) {
  return {
    ...value,
    // remove fields we do not need to expose
    deviceId: undefined,
    usage: undefined,
    // change null to undefined
    temperature: value.temperature ? value.temperature : undefined,
    humidity: value.humidity ? value.humidity : undefined,
    moisture: value.moisture ? value.moisture : undefined,
    ph: value.ph ? value.ph : undefined,
  };
}

function simplifyTelemetryActuators(value: DeviceTelemetryActuation) {
  return {
    ...value,
    // remove fields we do not need to expose
    deviceId: undefined,
    usage: undefined,
    // change null to undefined
    fan: value.fan ? value.fan : undefined,
    pump: value.pump ? value.pump : undefined,
  };
}

app.get('/:id/telemetry/sensors', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id } });
  if (!device) return context.json({ error: 'device_not_found' }, 400);
  const telemetries = await prisma.deviceTelemetrySensors.findMany({
    where: { deviceId: device.id },
    orderBy: { created: 'desc' },
    take: 100,
  });
  return context.json(telemetries.map(simplifyTelemetrySensors));
});

app.get('/:id/telemetry/sensors/latest', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id } });
  if (!device) return context.json({ error: 'device_not_found' }, 400);
  const telemetry = await prisma.deviceTelemetrySensors.findFirst({
    where: { deviceId: device.id },
    orderBy: { created: 'desc' },
    take: 1,
  });
  return telemetry ? context.json(simplifyTelemetrySensors(telemetry)) : context.body(null, 204);
});

app.get('/:id/telemetry/actuators', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id } });
  if (!device) return context.json({ error: 'device_not_found' }, 400);
  const telemetries = await prisma.deviceTelemetryActuation.findMany({
    where: { deviceId: device.id },
    orderBy: { created: 'desc' },
    take: 100,
  });
  return context.json(telemetries.map(simplifyTelemetryActuators));
});

app.get('/:id/telemetry/actuators/latest', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id } });
  if (!device) return context.json({ error: 'device_not_found' }, 400);
  const telemetry = await prisma.deviceTelemetryActuation.findFirst({
    where: { deviceId: device.id },
    orderBy: { created: 'desc' },
    take: 1,
  });
  return telemetry ? context.json(simplifyTelemetryActuators(telemetry)) : context.body(null, 204);
});

export const GET = handle(app);
