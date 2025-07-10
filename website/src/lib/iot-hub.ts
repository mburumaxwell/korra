// The module 'azure-iothub' is a CommonJS module, which may not support all module.exports as named exports.
// CommonJS modules can always be imported via the default export.
import iothub from 'azure-iothub';
const { Registry } = iothub;

import { type KorraDeviceTwin, KorraDeviceTwinSchema } from './schemas.ts';

let defaultRegistry: iothub.Registry | undefined = undefined;

// we have a function instead of a constant because
// when loading from scripts, we want to lowe only when need it (necessary when not under src folder)
// when UI mode, we do not want it compiled
export function getRegistry(connectionString?: string): iothub.Registry {
  if (defaultRegistry) return defaultRegistry;
  if (!connectionString) {
    return (defaultRegistry = Registry.fromConnectionString(process.env.IOT_HUB_CONNECTION_STRING!));
  }
  return Registry.fromConnectionString(connectionString);
}

export async function getTwin(registry: iothub.Registry, deviceId: string): Promise<KorraDeviceTwin> {
  return KorraDeviceTwinSchema.parse((await registry.getTwin(deviceId)).responseBody);
}
