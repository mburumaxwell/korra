import { Hono } from 'hono';
import { bearerAuth } from 'hono/bearer-auth';
import { handle } from 'hono/vercel';

import { prisma } from '@/lib/prisma';
import { type DeviceTelemetry } from '@/lib/prisma/client';

export const dynamic = 'force-dynamic';

const app = new Hono().basePath('/api/devices');
app.use('/*', bearerAuth({ token: `${process.env.PROCESSOR_API_KEY}` }));

app.get('/', async (context) => {
  const devices = await prisma.device.findMany();
  return context.json(devices.map((d) => ({ ...d, certificatePem: undefined })));
});

app.get('/:id', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id } });
  return context.json(device);
});

function simplifyTelemetry(value: DeviceTelemetry) {
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

app.get('/:id/telemetry', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id } });
  if (!device) return context.json({ error: 'device_not_found' }, 400);
  const telemetries = await prisma.deviceTelemetry.findMany({
    where: { deviceId: device.id },
    orderBy: { created: 'desc' },
    take: 100,
  });
  return context.json(telemetries.map(simplifyTelemetry));
});

app.get('/:id/telemetry/latest', async (context) => {
  const id = context.req.param('id');
  const device = await prisma.device.findFirst({ where: { id } });
  if (!device) return context.json({ error: 'device_not_found' }, 400);
  const telemetry = await prisma.deviceTelemetry.findFirst({
    where: { deviceId: device.id },
    orderBy: { created: 'desc' },
    take: 1,
  });
  return telemetry ? context.json(simplifyTelemetry(telemetry)) : context.body(null, 204);
});

export const GET = handle(app);
