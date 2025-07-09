import type { Metadata, ResolvingMetadata } from 'next';
import { notFound } from 'next/navigation';

import { getDevice, getDeviceTelemetries } from '@/actions';
import { getDateTimeRange, granularityToMilliseconds, type Granularity, type TimeRange } from '@/lib/aggregation';
import { DeviceInformation, DeviceViewHeader, DeviceViewHistoryChart } from './device-view';

export const revalidate = 0; // no caching

const defaultRange: TimeRange = '6h';
const defaultGranularity: Granularity = '15m';

type ViewDevicePageProps = {
  params: Promise<{ id: string }>;
  searchParams: Promise<{ range?: TimeRange; granularity?: Granularity }>;
};

export async function generateMetadata(props: ViewDevicePageProps, parent: ResolvingMetadata): Promise<Metadata> {
  const params = await props.params;
  const { id } = params;
  const device = await getDevice(id);
  if (!device) {
    notFound();
  }

  return {
    title: device.label,
    description: `Detailed view for ${device.label} (${device.id})`,
  };
}

export default async function Page(props: ViewDevicePageProps) {
  const params = await props.params;
  const { id } = params;
  const device = await getDevice(id);
  if (!device) {
    notFound();
  }

  const searchParams = await props.searchParams;
  const { range = defaultRange, granularity = defaultGranularity } = searchParams;
  const { start, end } = getDateTimeRange(range);
  const granularityMs = granularityToMilliseconds(granularity);
  const telemetries = await getDeviceTelemetries({ deviceId: device.id, start, end, granularity: granularityMs });

  return (
    <div className="container mx-auto p-6 space-y-6">
      <DeviceViewHeader device={device} />
      <DeviceInformation device={device} />
      <DeviceViewHistoryChart
        device={device}
        defaultRange={defaultRange}
        defaultGranularity={defaultGranularity}
        telemetries={telemetries}
      />
    </div>
  );
}
