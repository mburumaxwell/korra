import { type Metadata } from 'next';

import { getDevices } from '@/actions';
import { DeviceList } from './device-list';

export const revalidate = 0; // no caching

export const metadata: Metadata = {
  title: 'Devices',
  description: 'Devices managed in Korra',
};

export default async function Page() {
  const devices = await getDevices();

  return <DeviceList devices={devices}></DeviceList>;
}
