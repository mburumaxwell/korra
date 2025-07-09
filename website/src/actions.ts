'use server';

import { prisma } from '@/lib/prisma';
import { type Device, type DeviceFirmware, type DeviceTelemetry } from '@/lib/prisma/client';

export type DisplayableDevice = Device & { firmware?: DeviceFirmware | null; latestTelemetry?: DeviceTelemetry | null };

export async function getDevices(): Promise<DisplayableDevice[]> {
  const devices = await prisma.device.findMany({
    include: { firmware: true },
  });
  const results = await Promise.all(
    devices.map(async (device): Promise<DisplayableDevice> => {
      const latestTelemetry = await prisma.deviceTelemetry.findFirst({
        where: { deviceId: device.id },
        orderBy: { created: 'desc' },
        take: 1,
      });
      return {
        ...device,
        latestTelemetry,
      } satisfies DisplayableDevice;
    }),
  );
  return results;
}

// export async function createDevice(device: Omit<Device, "id" | "lastSeen">): Promise<Device> {
//   const newDevice: Device = {
//     ...device,
//     id: Date.now().toString(),
//     lastSeen: new Date().toISOString(),
//   }
//   return newDevice
// }

// export async function updateDevice(id: string, updates: Partial<Device>): Promise<Device | null> {
//   return { ...updates, id } as Device
// }

// export async function deleteDevice(id: string): Promise<boolean> {
//   return true
// }
