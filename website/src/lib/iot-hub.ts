// The module 'azure-iothub' is a CommonJS module, which may not support all module.exports as named exports.
// CommonJS modules can always be imported via the default export.
import iothub from 'azure-iothub';
const { Registry } = iothub;

const connectionString = process.env.IOT_HUB_CONNECTION_STRING!;
export const registry = Registry.fromConnectionString(connectionString);
