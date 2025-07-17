'use server';

import { prisma } from '@/lib/prisma';
import {
  type AvailableFirmware,
  type BoardType,
  type Device,
  type DeviceFirmware,
  type DeviceNetwork,
  type DeviceTelemetry,
  type DeviceUsage,
  type FirmwareFramework,
} from '@/lib/prisma/client';
import { KORRA_BOARD_TYPES, KORRA_FIRMWARE_FRAMEWORKS, KORRA_USAGE_TYPES } from '@/lib/schemas';

export type DisplayableFirmware = AvailableFirmware & {};

export type GetFirmwareProps = {
  usage?: DeviceUsage;
  board?: BoardType;
  framework?: FirmwareFramework;
};

export async function getAvailableFirmware(props: GetFirmwareProps): Promise<DisplayableFirmware[]> {
  const firmware = await prisma.availableFirmware.findMany({
    where: {
      ...(props.usage ? { usage: props.usage } : {}),
      ...(props.board ? { board: props.board } : {}),
      ...(props.framework ? { framework: props.framework } : {}),
    },
    orderBy: { created: 'desc' },
  });
  return firmware as DisplayableFirmware[];
}

export async function getLatestAvailableFirmware(): Promise<DisplayableFirmware[]> {
  return (
    await Promise.all(
      KORRA_BOARD_TYPES.flatMap((board) => {
        return KORRA_USAGE_TYPES.flatMap((usage) => {
          return KORRA_FIRMWARE_FRAMEWORKS.map(async (framework) => {
            const latest = await prisma.availableFirmware.findFirst({
              where: { board, usage, framework },
              orderBy: { versionValue: 'desc' },
            });
            return latest;
          });
        });
      }),
    )
  ).filter((f) => f !== null) as DisplayableFirmware[];
}

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
  const latestTelemetry = await prisma.deviceTelemetry.findFirst({
    where: { deviceId: device.id },
    orderBy: { created: 'desc' },
    take: 1,
  });

  return { ...device, latestTelemetry };
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

export type BucketedDeviceTelemetry = {
  bucket: Date;
  temperature: number | null;
  humidity: number | null;
  ph: number | null;
  moisture: number | null;
};

export type GetDeviceTelemetriesProps = {
  deviceId: string;
  start: Date;
  end: Date;
  granularity: number;
};

export async function getDeviceTelemetries(props: GetDeviceTelemetriesProps): Promise<BucketedDeviceTelemetry[]> {
  // fetch all points in window
  const { deviceId, start, end, granularity } = props;
  const telemetries = await prisma.deviceTelemetry.findMany({
    where: { deviceId, created: { gte: start, lte: end } },
  });

  // bucket accumulators
  type Agg = { sum: number; count: number };
  const buckets = new Map<number, { temperature: Agg; humidity: Agg; ph: Agg; moisture: Agg }>();

  // // 2) Bucket & average
  for (const { created, temperature, humidity, moisture, ph } of telemetries) {
    const offsetMs = created.getTime() - start.getTime();
    const bucketIndex = Math.floor(offsetMs / granularity);
    const bucketStartMs = start.getTime() + bucketIndex * granularity;

    let agg = buckets.get(bucketStartMs);
    if (!agg) {
      agg = {
        temperature: { sum: 0, count: 0 },
        humidity: { sum: 0, count: 0 },
        ph: { sum: 0, count: 0 },
        moisture: { sum: 0, count: 0 },
      };
      buckets.set(bucketStartMs, agg);
    }

    if (temperature != null) {
      agg.temperature.sum += temperature;
      agg.temperature.count += 1;
    }
    if (humidity != null) {
      agg.humidity.sum += humidity;
      agg.humidity.count += 1;
    }
    if (ph != null) {
      agg.ph.sum += ph;
      agg.ph.count += 1;
    }
    if (moisture != null) {
      agg.moisture.sum += moisture;
      agg.moisture.count += 1;
    }
  }

  // turn into sorted array of averages
  const result: BucketedDeviceTelemetry[] = Array.from(buckets.entries())
    .map(
      ([ms, { temperature: temp, humidity: humid, ph, moisture: moist }]): BucketedDeviceTelemetry => ({
        bucket: new Date(ms),
        temperature: temp.count > 0 ? temp.sum / temp.count : null,
        humidity: humid.count > 0 ? humid.sum / humid.count : null,
        ph: ph.count > 0 ? ph.sum / ph.count : null,
        moisture: moist.count > 0 ? moist.sum / moist.count : null,
      }),
    )
    .sort((a, b) => a.bucket.getTime() - b.bucket.getTime());

  return result;
}
