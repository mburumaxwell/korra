import type { Metadata, ResolvingMetadata } from 'next';
import { notFound } from 'next/navigation';

import { getDevice, getDeviceActuations, getDeviceTelemetries } from '@/actions';
import { getDateTimeRange, granularityToMilliseconds, type Granularity, type TimeRange } from '@/lib/aggregation';
import { DeviceActuationsTable, DeviceInformation, DeviceSensorsHistory, DeviceViewHeader } from './device-view';

export const revalidate = 0; // no caching

const defaultRange: TimeRange = '6h';
const defaultGranularity: Granularity = '15m';

export async function generateMetadata(
  props: PageProps<'/dashboard/devices/[id]'>,
  parent: ResolvingMetadata,
): Promise<Metadata> {
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

export default async function Page(props: PageProps<'/dashboard/devices/[id]'>) {
  const params = await props.params;
  const { id } = params;
  const device = await getDevice(id);
  if (!device) {
    notFound();
  }

  const searchParams = await props.searchParams;
  const { range = defaultRange, granularity = defaultGranularity } = searchParams as {
    range?: TimeRange;
    granularity?: Granularity;
  };
  const { start, end } = getDateTimeRange(range);
  const granularityMs = granularityToMilliseconds(granularity);
  const sensors = await getDeviceTelemetries({ deviceId: device.id, start, end, granularity: granularityMs });
  const actuations = await getDeviceActuations({ deviceId: device.id });

  return (
    <>
      <DeviceViewHeader device={device} />
      <DeviceInformation device={device} />
      <DeviceSensorsHistory
        device={device}
        defaultRange={defaultRange}
        defaultGranularity={defaultGranularity}
        data={sensors}
      />
      <DeviceActuationsTable device={device} data={actuations} />
    </>
  );
}
