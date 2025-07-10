import { Cable, Flower2, House, Smartphone, Wifi } from 'lucide-react';

import { type KorraNetworkKind, type KorraUsageType } from '@/lib/schemas';

export function getDeviceIcon(usage: KorraUsageType) {
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
