import { getDevices } from '@/actions';
import { DeviceList } from './device-list';

export default async function Page() {
  const devices = await getDevices();

  return <DeviceList devices={devices}></DeviceList>;
}
