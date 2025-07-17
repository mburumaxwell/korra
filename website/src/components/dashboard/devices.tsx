import { Cable, Flower2, House, Smartphone, Wifi } from 'lucide-react';

import {
  type KorraBoardType,
  type KorraFirmwareFramework,
  type KorraNetworkKind,
  type KorraUsageType,
} from '@/lib/schemas';

export function getUsageIcon(usage: KorraUsageType) {
  return usage === 'keeper' ? House : Flower2;
}

export function getNetworkIcon(kind?: KorraNetworkKind | null) {
  switch (kind) {
    case 'wifi':
      return Wifi;
    case 'ethernet':
      return Cable;
    case 'cellular':
      return Smartphone;
    default:
      return Wifi;
  }
}

export function getBoardName(board: KorraBoardType) {
  switch (board) {
    case 'esp32s3_devkitc':
      return 'ESP32-S3 DevKitC';
    case 'frdm_rw612':
      return 'FRDM-RW612';
    case 'nrf7002dk':
      return 'nRF7002 DK';
    default:
      throw new Error(`Unknown board type: ${board}`);
  }
}

export function getFrameworkName(framework: KorraFirmwareFramework) {
  switch (framework) {
    case 'arduino':
      return 'Arduino';
    case 'zephyr':
      return 'Zephyr';
    case 'espidf':
      return 'ESP-IDF';
    default:
      throw new Error(`Unknown firmware framework: ${framework}`);
  }
}
