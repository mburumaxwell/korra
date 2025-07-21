'use client';

import { Copy, Droplets, Zap } from 'lucide-react';
import Link from 'next/link';
import { usePathname, useRouter, useSearchParams } from 'next/navigation';

import type { BucketedDeviceTelemetry, DisplayableDevice, DisplayableDeviceActuation } from '@/actions';
import { getNetworkIcon, getUsageIcon } from '@/components/dashboard/devices';
import { TelemetryChart, type TelemetryChartConfig } from '@/components/telemetry-chart';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Separator } from '@/components/ui/separator';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '@/components/ui/table';
import { granularityOptions, timeRangeOptions, type Granularity, type TimeRange } from '@/lib/aggregation';
import { copyToClipboard } from '@/lib/utils';

export function DeviceViewHeader({ device }: { device: DisplayableDevice }) {
  const UsageIcon = getUsageIcon(device.usage);
  return (
    <div className="flex items-center space-x-4">
      <div className="flex items-center space-x-2">
        <UsageIcon className="text-muted-foreground h-6 w-6" />
        <h1 className="text-3xl font-bold">{device.label}</h1>
        <Badge variant="outline" className="text-sm">
          {device.usage}
        </Badge>
      </div>
    </div>
  );
}

export function DeviceInformation({ device }: { device: DisplayableDevice }) {
  const NetworkIcon = getNetworkIcon(device.network?.kind);
  const { latestTelemetry } = device;

  return (
    <Card>
      <CardHeader>
        <CardTitle>Device Information</CardTitle>
        <CardDescription>Current status and configuration details</CardDescription>
      </CardHeader>
      <CardContent className="space-y-6">
        <div className="grid grid-cols-2 gap-6 md:grid-cols-3 lg:grid-cols-4">
          {/* Device ID */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">Device ID</h3>
            <div className="flex items-center space-x-2">
              <p className="truncate font-mono text-sm">{device.id}</p>
              <Button
                variant="ghost"
                size="sm"
                onClick={() => copyToClipboard(device.id)}
                className="h-6 w-6 p-0 hover:cursor-pointer"
              >
                <Copy className="h-3 w-3" />
              </Button>
            </div>
          </div>

          {/* Status */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">Status</h3>
            <Badge variant={device.connected ? 'default' : 'destructive'} className="text-sm">
              {device.connected ? 'Online' : 'Offline'}
            </Badge>
          </div>

          {/* Last Seen */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">Last Seen</h3>
            {/* suppressHydrationWarning is set because SSR and client render tend to produce different results */}
            <p className="text-sm" suppressHydrationWarning>
              {device.lastSeen?.toLocaleString()}
            </p>
          </div>

          {/* Board */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">Board</h3>
            <Link href={`/boards#${device.board}`} className="underline-offset-4 hover:underline" target="_blank">
              <p className="font-mono text-sm">{device.board}</p>
            </Link>
          </div>

          {/* Framework */}
          {device.framework && (
            <div className="space-y-2">
              <h3 className="text-muted-foreground text-sm font-medium">Framework</h3>
              <Link
                href={`/frameworks#${device.framework}`}
                className="underline-offset-4 hover:underline"
                target="_blank"
              >
                <p className="font-mono text-sm">{device.framework}</p>
              </Link>
            </div>
          )}

          {/* Firmware */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">Firmware</h3>
            <p className="font-mono text-sm">
              {device.firmware?.currentVersion ? `v${device.firmware?.currentVersion}` : '—'}
            </p>
          </div>

          {/* Network Type */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">Network Type</h3>
            <div className="flex items-center space-x-2">
              <NetworkIcon className="h-4 w-4" />
              <span className="text-sm capitalize">{device.network?.kind || 'Unknown'}</span>
            </div>
          </div>

          {/* Local IP Address */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">Local IP Address</h3>
            <div className="flex items-center space-x-2">
              <p className="truncate font-mono text-sm">{device.network?.localIp || '—'}</p>
              {device.network?.localIp && (
                <Button
                  variant="ghost"
                  size="sm"
                  onClick={() => copyToClipboard(device.network!.localIp!)}
                  className="h-6 w-6 p-0 hover:cursor-pointer"
                >
                  <Copy className="h-3 w-3" />
                </Button>
              )}
            </div>
          </div>

          {/* MAC Address */}
          <div className="space-y-2">
            <h3 className="text-muted-foreground text-sm font-medium">MAC Address</h3>
            <div className="flex items-center space-x-2">
              <p className="font-mono text-sm">{device.network?.mac || '—'}</p>
              {device.network?.mac && (
                <Button
                  variant="ghost"
                  size="sm"
                  onClick={() => copyToClipboard(device.network!.mac!)}
                  className="h-6 w-6 p-0 hover:cursor-pointer"
                >
                  <Copy className="h-3 w-3" />
                </Button>
              )}
            </div>
          </div>

          {/* Network Name */}
          {device.network?.name && (
            <div className="space-y-2">
              <h3 className="text-muted-foreground text-sm font-medium">Network Name</h3>
              <p className="truncate text-sm">{device.network.name}</p>
            </div>
          )}
        </div>

        {/* Current Sensor Values */}
        <Separator />
        <div>
          <h3 className="mb-4 text-lg font-medium">Current Sensor Values</h3>
          <div className="grid grid-cols-1 gap-4 md:grid-cols-2">
            {device.usage === 'keeper' ? (
              <>
                <div className="rounded-lg border p-4">
                  <h4 className="text-muted-foreground text-sm font-medium">Temperature</h4>
                  <p className="text-2xl font-bold">
                    {latestTelemetry?.temperature ? `${latestTelemetry?.temperature}°C` : '—'}
                  </p>
                </div>
                <div className="rounded-lg border p-4">
                  <h4 className="text-muted-foreground text-sm font-medium">Humidity</h4>
                  <p className="text-2xl font-bold">
                    {latestTelemetry?.humidity ? `${latestTelemetry?.humidity}% RH` : '—'}
                  </p>
                </div>
              </>
            ) : (
              <>
                <div className="rounded-lg border p-4">
                  <h4 className="text-muted-foreground text-sm font-medium">Soil Moisture</h4>
                  <p className="text-2xl font-bold">
                    {latestTelemetry?.moisture ? `${latestTelemetry?.moisture.toFixed(2)}%` : '—'}
                  </p>
                </div>
                <div className="rounded-lg border p-4">
                  <h4 className="text-muted-foreground text-sm font-medium">pH Level</h4>
                  <p className="text-2xl font-bold">
                    {latestTelemetry?.ph ? `${latestTelemetry?.ph.toFixed(2)}` : '—'}
                  </p>
                </div>
              </>
            )}
          </div>
        </div>
      </CardContent>
    </Card>
  );
}

type DeviceSensorsHistoryProps = {
  device: DisplayableDevice;
  defaultRange: TimeRange;
  defaultGranularity: Granularity;
  data: BucketedDeviceTelemetry[];
};

export function DeviceSensorsHistory({ device, defaultRange, defaultGranularity, data }: DeviceSensorsHistoryProps) {
  const searchParams = useSearchParams();
  const router = useRouter();
  const pathname = usePathname();

  const range = (searchParams.get('range') as TimeRange) ?? defaultRange;
  const granularity = (searchParams.get('granularity') as Granularity) ?? defaultGranularity;

  function updateParams(updates: { range?: string; granularity?: string }) {
    const params = new URLSearchParams(Array.from(searchParams.entries()));
    if (updates.range != null) params.set('range', updates.range);
    if (updates.granularity != null) params.set('granularity', updates.granularity);
    router.push(pathname + '?' + params.toString());
  }

  function formatTimestamp(date: Date) {
    if (range === '1h' || range === '6h') {
      return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    }
    return date.toLocaleDateString([], { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit' });
  }

  const metrics: TelemetryChartConfig[] = [];
  if (device.usage === 'keeper') {
    metrics.push(
      ...([
        {
          key: 'temperature',
          title: 'Temperature (°C)',
          // domain: ['dataMin - 2', 'dataMax + 2'],
          domain: [-1, 50], // fixed from –1°C up to 50°C
          decimals: 1,
          suffix: '°C',
        },
        {
          key: 'humidity',
          title: 'Humidity (% RH)',
          domain: [0, 100],
          decimals: 1,
          suffix: '%',
        },
      ] satisfies TelemetryChartConfig[]),
    );
  } else if (device.usage === 'pot') {
    metrics.push(
      ...([
        {
          key: 'moisture',
          title: 'Soil Moisture (%)',
          domain: [0, 100],
          decimals: 1,
          suffix: '%',
        },
        {
          key: 'ph',
          title: 'pH Level',
          domain: [5, 8],
          decimals: 2,
          suffix: '',
        },
      ] satisfies TelemetryChartConfig[]),
    );
  }

  return (
    <>
      {/* Chart Controls */}
      <Card>
        <CardHeader>
          <div className="flex flex-col space-y-2 sm:flex-row sm:items-center sm:justify-between sm:space-y-0">
            <div>
              <CardTitle>Sensor Data</CardTitle>
              <CardDescription>Historical sensor readings over time</CardDescription>
            </div>
            <div className="flex flex-col space-y-2 sm:flex-row sm:space-y-0 sm:space-x-2">
              <Select value={range} onValueChange={(value: TimeRange) => updateParams({ range: value })}>
                <SelectTrigger className="w-[140px]">
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  {timeRangeOptions.map((option) => (
                    <SelectItem key={option.value} value={option.value}>
                      {option.label}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
              <Select value={granularity} onValueChange={(value: Granularity) => updateParams({ granularity: value })}>
                <SelectTrigger className="w-[140px]">
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  {granularityOptions.map((option) => (
                    <SelectItem key={option.value} value={option.value}>
                      {option.label}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>
          </div>
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-1 gap-4 space-y-6 md:grid-cols-2">
            {metrics.map((cfg) => (
              <TelemetryChart key={cfg.key} data={data} tickFormatter={formatTimestamp} config={cfg} />
            ))}
          </div>
        </CardContent>
      </Card>
    </>
  );
}

type DeviceActuationsTableProps = {
  device: DisplayableDevice;
  data: DisplayableDeviceActuation[];
};

export function DeviceActuationsTable({ device, data }: DeviceActuationsTableProps) {
  if (data.length === 0) return <></>;

  return (
    <div>
      <h3 className="mb-4 text-lg font-medium">Recent Actuations</h3>
      <div className="rounded-lg border">
        <Table>
          <TableHeader>
            <TableRow>
              <TableHead>Timestamp</TableHead>
              <TableHead>Type</TableHead>
              <TableHead>Duration</TableHead>
              <TableHead>Status</TableHead>
            </TableRow>
          </TableHeader>
          <TableBody>
            {data.map((actuation) => {
              // const latency = actuation.received ? actuation.received.getTime() - actuation.created.getTime() : null

              return (
                <TableRow key={actuation.id}>
                  <TableCell className="text-sm">
                    <div>
                      <div className="font-medium" suppressHydrationWarning>
                        {actuation.created.toLocaleString()}
                      </div>
                      {/* {latency && <div className="text-xs text-muted-foreground">Latency: {latency}ms</div>} */}
                    </div>
                  </TableCell>
                  <TableCell>
                    <div className="flex items-center space-x-2">
                      {device.usage === 'keeper' ? (
                        <>
                          <Zap className="h-4 w-4 text-blue-500" />
                          <span>Fan</span>
                        </>
                      ) : (
                        <>
                          <Droplets className="h-4 w-4 text-green-500" />
                          <span>Pump</span>
                        </>
                      )}
                    </div>
                  </TableCell>
                  <TableCell className="font-mono">{actuation.fan ?? actuation.pump}s</TableCell>
                  <TableCell>
                    <Badge variant={'default'}>{'Completed'}</Badge>
                  </TableCell>
                </TableRow>
              );
            })}
          </TableBody>
        </Table>
      </div>
    </div>
  );
}
