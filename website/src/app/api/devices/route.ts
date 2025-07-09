// this route exists only as a means to test that the API is working
// last attempt at validating the use of Azure SWA vs any other platform/host

import { Hono } from 'hono';
import { bearerAuth } from 'hono/bearer-auth';
import { handle } from 'hono/vercel';

import { prisma } from '@/lib/prisma';

export const dynamic = 'force-dynamic';

const app = new Hono().basePath('/api/devices');
app.use('/*', bearerAuth({ token: `${process.env.PROCESSOR_API_KEY}` }));

app.get('/', async (context) => {
  const devices = await prisma.device.findMany();
  return context.json(devices);
});

export const GET = handle(app);
