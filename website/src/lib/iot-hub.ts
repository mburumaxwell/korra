import { Registry } from 'azure-iothub';
import { z } from 'zod/v4';
import {
  KorraActuatorConfigSchema,
  KorraActuatorStateSchema,
  KorraFirmwareVersionSchema,
  KorraNetworkInfoSchema,
} from './schemas';

export const KorraDeviceTwinDesiredFirmwareSchema = z.object({
  version: KorraFirmwareVersionSchema.nullish(), // version
  url: z.url(), // firmware binary URL
  hash: z.string(), // SHA-256 hash in hex
  signature: z.string().nullish(), // signature (e.g., base64 or hex)
});
export type KorraDeviceTwinDesiredFirmware = z.infer<typeof KorraDeviceTwinDesiredFirmwareSchema>;

export const KorraDeviceTwinDesiredSchema = z.object({
  $version: z.number().int(),
  firmware: KorraDeviceTwinDesiredFirmwareSchema.nullish(),
  actuator: KorraActuatorConfigSchema.nullish(),
});
export type KorraDeviceTwinDesired = z.infer<typeof KorraDeviceTwinDesiredSchema>;

export const KorraDeviceTwinReportedFirmwareSchema = z.object({
  version: KorraFirmwareVersionSchema.nullish(), // version
});
export type KorraDeviceTwinReportedFirmware = z.infer<typeof KorraDeviceTwinReportedFirmwareSchema>;

export const KorraDeviceTwinReportedSchema = z.object({
  $version: z.number().int(),
  firmware: KorraDeviceTwinReportedFirmwareSchema.nullish(),
  actuator: KorraActuatorStateSchema.nullish(),
  network: KorraNetworkInfoSchema.nullish(),
});
export type KorraDeviceTwinReported = z.infer<typeof KorraDeviceTwinReportedSchema>;

export const KorraDeviceTwinSchema = z.object({
  desired: KorraDeviceTwinDesiredSchema,
  reported: KorraDeviceTwinReportedSchema,
});
export type KorraDeviceTwin = z.infer<typeof KorraDeviceTwinSchema>;

export async function getDeviceTwin(deviceId: string): Promise<KorraDeviceTwin | undefined> {
  const connectionString = process.env.IOT_HUB_CONNECTION_STRING!;
  const registry = Registry.fromConnectionString(connectionString);

  const response = await registry.getTwin(deviceId);
  const parsed = KorraDeviceTwinSchema.safeParse(response.responseBody);
  return parsed.success ? parsed.data : undefined;
}
