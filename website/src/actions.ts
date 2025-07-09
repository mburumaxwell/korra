'use server';

import { prisma } from '@/lib/prisma';
import { type Device, type DeviceFirmware, type DeviceNetwork, type DeviceTelemetry } from '@/lib/prisma/client';

export type DisplayableDevice = Device & {
  firmware?: DeviceFirmware | null;
  latestTelemetry?: DeviceTelemetry | null;
  network?: DeviceNetwork | null;
};

export async function getDevices(): Promise<DisplayableDevice[]> {
  const devices = await prisma.device.findMany({
    include: { firmware: true, network: true },
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

  // Sort in place
  const usageOrder = { keeper: 0, pot: 1 };
  results.sort((a, b) => {
    // Compare usage first
    const u = usageOrder[a.usage] - usageOrder[b.usage];
    if (u !== 0) return u;

    // Same usage → compare labels alphanumerically
    // numeric:true makes "K2" < "K10" work correctly
    return a.label.localeCompare(b.label, undefined, { numeric: true });
  });

  return results;
}

export async function getDevice(id: string): Promise<DisplayableDevice> {
  const device = await prisma.device.findUniqueOrThrow({
    where: { id: id },
    include: { firmware: true, network: true },
  });
  return device;
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
