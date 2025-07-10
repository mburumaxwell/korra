import { z } from 'zod/v4';

export const KorraActuatorConfigSchema = z.object({
  enabled: z.boolean(), // Whether or not the actuator is allowed
  duration: z.number().min(5).max(60), // Seconds for which the actuator should be active at a given time (range: 5-60)
  equilibrium_time: z.number().min(3).max(60), // Seconds to wait before the next actuation (range: 3-60)
  target: z.number(),
});
export type KorraActuatorConfig = z.infer<typeof KorraActuatorConfigSchema>;

export const KorraActuatorStateSchema = z.object({
  count: z.number().int(), // Number of times the actuator was activated
  last_time: z.number(), // Last time (UNIX since Epoch) the actuator was activated
  total_duration: z.number().int(), // Total seconds the actuator was active
});
export type KorraActuatorState = z.infer<typeof KorraActuatorStateSchema>;

export const KorraNetworkKindSchema = z.enum(['wifi', 'ethernet', 'cellular']);
export type KorraNetworkKind = z.infer<typeof KorraNetworkKindSchema>;

export const KorraNetworkInfoSchema = z.object({
  kind: z.enum(['wifi', 'ethernet', 'cellular']),
  mac: z.string(), // The MAC address of the device.
  name: z.string().nullish(), // The name of the network.
  local_ip: z.string(), // The local IP address.
});
export type KorraNetworkInfo = z.infer<typeof KorraNetworkInfoSchema>;

export const KorraFirmwareVersionSchema = z.object({
  value: z.number(),
  semver: z.string(),
});
export type KorraFirmwareVersion = z.infer<typeof KorraFirmwareVersionSchema>;

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

export const KorraUsageTypeSchema = z.enum(['keeper', 'pot']);
export type KorraUsageType = z.infer<typeof KorraUsageTypeSchema>;
export const KORRA_USAGE_TYPES = ['pot', 'keeper'] as const;

export const KorraDeviceTwinTagsSchema = z.object({
  usage: KorraUsageTypeSchema.nullish(),
  label: z.enum(['K1', 'D1', 'D2', 'D3', 'M1', 'M2', 'M3', 'W1', 'W2', 'W3']).or(z.string()).nullish(),
});
export type KorraDeviceTwinTags = z.infer<typeof KorraDeviceTwinTagsSchema>;

export const KorraDeviceTwinSchema = z.object({
  deviceId: z.string(),
  tags: KorraDeviceTwinTagsSchema,
  properties: z.object({
    desired: KorraDeviceTwinDesiredSchema,
    reported: KorraDeviceTwinReportedSchema,
  }),
  etag: z.string(),
});
export type KorraDeviceTwin = z.infer<typeof KorraDeviceTwinSchema>;

export const KorraTelemetryKeeperSchema = z.object({
  app_kind: z.literal('keeper'),
  temperature: z.number(), // °C
  humidity: z.number(), // Relative humidity (%)
});
export type KorraTelemetryKeeper = z.infer<typeof KorraTelemetryKeeperSchema>;

export const KorraTelemetryPotSchema = z.object({
  app_kind: z.literal('pot'),
  moisture: z.number(), // Percentage (%) of water in the soil
  ph: z.number().nullish(),
});
export type KorraTelemetryPot = z.infer<typeof KorraTelemetryPotSchema>;

export const KorraTelemetrySchema = z.discriminatedUnion('app_kind', [
  KorraTelemetryKeeperSchema,
  KorraTelemetryPotSchema,
]);
export type KorraTelemetry = z.infer<typeof KorraTelemetrySchema>;

export const TelemetryRequestBodySchema = z.intersection(
  z.object({
    id: z.string(),
    device_id: z.string(),
    created: z.coerce.date(),
    received: z.coerce.date().nullish(),
  }),
  KorraTelemetrySchema,
);
export type TelemetryRequestBody = z.infer<typeof TelemetryRequestBodySchema>;

export const OperationalEventRequestBodySchema = z.object({
  type: z.enum(['connected', 'disconnected', 'twin.updated']),
  device_id: z.string(),
  sequence_number: z.string().nullish(),
  received: z.coerce.date().nullish(),
});
export type OperationalEventRequestBody = z.infer<typeof OperationalEventRequestBodySchema>;

export const KorraBoardTypeSchema = z.enum(['esp32s3_devkitc', 'frdm_rw612', 'nrf7002dk']);
export type KorraBoardType = z.infer<typeof KorraBoardTypeSchema>;
export const KORRA_BOARD_TYPES = ['esp32s3_devkitc', 'frdm_rw612', 'nrf7002dk'] as const;

export const KorraFirmwareFrameworkSchema = z.enum(['zephyr', 'arduino', 'espidf']);
export type KorraFirmwareFramework = z.infer<typeof KorraFirmwareFrameworkSchema>;

export const AvailableFirmwareRequestBodySchema = z.object({
  board: KorraBoardTypeSchema,
  usage: KorraUsageTypeSchema,
  framework: KorraFirmwareFrameworkSchema,
  version: KorraFirmwareVersionSchema, // version
  url: z.url(), // firmware binary URL
  attestation: z.url(), // attestation URL
  hash: z.string(), // SHA-256 hash in hex
  signature: z.string(), // signature (e.g., base64 or hex)
});
export type AvailableFirmwareRequestBody = z.infer<typeof AvailableFirmwareRequestBodySchema>;
