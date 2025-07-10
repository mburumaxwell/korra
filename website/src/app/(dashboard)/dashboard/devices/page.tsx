import { Plus } from 'lucide-react';
import { type Metadata } from 'next';
import Link from 'next/link';

import { getDevices } from '@/actions';
import { Button } from '@/components/ui/button';
import { DeviceList } from './device-list';

export const revalidate = 0; // no caching

export const metadata: Metadata = {
  title: 'Devices',
  description: 'Devices managed in Korra',
};

export default async function Page() {
  const devices = await getDevices();

  return (
    <>
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold">Devices</h1>
          <p className="text-muted-foreground">Manage devices</p>
        </div>
        <Button asChild>
          <Link href="/dashboard/devices/create">
            <Plus className="mr-2 h-4 w-4" />
            Add Device
          </Link>
        </Button>
      </div>

      <DeviceList devices={devices}></DeviceList>
    </>
  );
}
