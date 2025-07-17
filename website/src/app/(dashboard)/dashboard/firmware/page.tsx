import { type Metadata } from 'next';

import { getAvailableFirmware, getDevices, getLatestAvailableFirmware } from '@/actions';
import { Tabs, TabsList, TabsTrigger } from '@/components/ui/tabs';
import { type KorraBoardType, type KorraFirmwareFramework, type KorraUsageType } from '@/lib/schemas';
import { AvailableFirmwareTab } from './tabs-available-firmware';
import { DevicesFirmwareTab } from './tabs-devices';

export const revalidate = 60; // cache for 60 seconds

type FirmwarePageProps = {
  searchParams: Promise<{ usage?: KorraUsageType; board?: KorraBoardType; framework?: KorraFirmwareFramework }>;
};

export const metadata: Metadata = {
  title: 'Firmware',
  description: 'Manage firmware versions',
};

export default async function Page(props: FirmwarePageProps) {
  const searchParams = await props.searchParams;
  const { usage = 'all', board = 'all', framework = 'all' } = searchParams;
  const entries = await getAvailableFirmware({
    usage: usage !== 'all' ? usage : undefined,
    board: board !== 'all' ? board : undefined,
    framework: framework !== 'all' ? framework : undefined,
  });
  const devices = await getDevices();
  const latestFirmwareEntries = await getLatestAvailableFirmware();

  return (
    <div className="container">
      <div className="space-y-6">
        <div>
          <h1 className="text-3xl font-bold">Firmware Management</h1>
          <p className="text-muted-foreground">Manage firmware versions and updates</p>
        </div>

        <Tabs defaultValue="devices" className="space-y-6">
          <TabsList className="grid w-full grid-cols-2">
            <TabsTrigger value="devices">Device Status</TabsTrigger>
            <TabsTrigger value="firmware">Available Firmware</TabsTrigger>
          </TabsList>

          <AvailableFirmwareTab
            value="firmware"
            defaultBoard={board}
            defaultUsage={usage}
            defaultFramework={framework}
            entries={entries}
          />
          <DevicesFirmwareTab value="devices" devices={devices} latestFirmwareEntries={latestFirmwareEntries} />
        </Tabs>
      </div>
    </div>
  );
}
