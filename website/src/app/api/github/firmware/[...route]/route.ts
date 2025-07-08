import { zValidator } from '@hono/zod-validator';
import { Hono } from 'hono';
import { bearerAuth } from 'hono/bearer-auth';
import { handle } from 'hono/vercel';

import { prisma } from '@/lib/prisma';
import { AvailableFirmwareRequestBodySchema } from '@/lib/schemas';

export const dynamic = 'force-dynamic';

const app = new Hono().basePath('/api/github/firmware');

app.use('/*', bearerAuth({ token: `${process.env.FIRMWARE_API_KEY}` }));

app.post('', zValidator('json', AvailableFirmwareRequestBodySchema), async (context) => {
  const payload = context.req.valid('json');
  console.log('Received new/updated firmware from github', payload);

  await prisma.availableFirmware.upsert({
    where: {
      board_usage_framework_versionSemver: {
        board: payload.board,
        usage: payload.usage,
        framework: payload.framework,
        versionSemver: payload.version.semver,
      },
    },
    create: {
      board: payload.board,
      usage: payload.usage,
      framework: payload.framework,
      versionSemver: payload.version.semver,

      versionValue: payload.version.value,
      url: payload.url,
      attestation: payload.attestation,
      hash: 'tbd', // TODO; pull signature from attestation URL
      signature: 'tbd', // TODO; pull signature from attestation URL
    },
    update: {
      versionValue: payload.version.value,
      url: payload.url,
      attestation: payload.attestation,
      hash: 'tbd', // TODO; pull signature from attestation URL
      signature: 'tbd', // TODO; pull signature from attestation URL
    },
  });

  return context.body(null, 201);
});

export const POST = handle(app);
