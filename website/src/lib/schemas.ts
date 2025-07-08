import { z } from 'zod/v4';

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
});
export type OperationalEventRequestBody = z.infer<typeof OperationalEventRequestBodySchema>;

export const KorraActuatorConfigSchema = z.object({
  enabled: z.boolean(), // Whether or not the actuator is allowed
  duration: z.number().min(5).max(15), // Seconds for which the actuator should be active at a given time (range: 5-15)
  equilibrium_time: z.number().min(5).max(60), // Seconds to wait before the next actuation (range: 5-60)
  target: z.number(),
});
export type KorraActuatorConfig = z.infer<typeof KorraActuatorConfigSchema>;

export const KorraActuatorStateSchema = z.object({
  count: z.number().int(), // Number of times the actuator was activated
  last_time: z.number(), // Last time (UNIX since Epoch) the actuator was activated
  total_duration: z.number().int(), // Total seconds the actuator was active
});
export type KorraActuatorState = z.infer<typeof KorraActuatorStateSchema>;

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

export const AvailableFirmwareRequestBodySchema = z.object({
  board: z.enum(['esp32s3_devkitc', 'frdm_rw612', 'nrf7002dk']),
  usage: z.enum(['keeper', 'pot']), // Device usage type
  framework: z.enum(['zephyr', 'arduino', 'espidf']),
  version: KorraFirmwareVersionSchema, // version
  url: z.url(), // firmware binary URL
  attestation: z.url(), // attestation URL (signature shall be pulled from this)
});
export type AvailableFirmwareRequestBody = z.infer<typeof AvailableFirmwareRequestBodySchema>;
