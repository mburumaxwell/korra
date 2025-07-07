import { PrismaClient } from '@prisma/client';

declare global {
  var prisma: PrismaClient | undefined;
}

export const prisma =
  global.prisma ||
  new PrismaClient({
    datasources: {
      db: {
        // The reason this is done here rather than in the .env file is because the Neon Vercel integration doesn't include it.
        url: `${process.env.DATABASE_URL}?connect_timeout=10&pool_timeout=10`,
      },
    },
  });

if (process.env.NODE_ENV === 'development') global.prisma = prisma;
