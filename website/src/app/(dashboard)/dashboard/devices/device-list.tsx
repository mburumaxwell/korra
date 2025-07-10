'use client';

import { Edit, Trash2 } from 'lucide-react';
import Link from 'next/link';
import { useRouter } from 'next/navigation';
import { useState } from 'react';

import type { DisplayableDevice } from '@/actions';
import { getDeviceIcon, getNetworkIcon } from '@/components/dashboard/devices';
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
  AlertDialogTrigger,
} from '@/components/ui/alert-dialog';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '@/components/ui/table';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';

export type DeviceListProps = {
  devices: DisplayableDevice[];
};

export function DeviceList({ devices: inputDevices }: DeviceListProps) {
  const [devices, setDevices] = useState<DisplayableDevice[]>(inputDevices);
  const router = useRouter();

  function handleDeleteDevice(id: string) {
    // TODO: do actual implementation once authentication/authorization is implemented
    // await deleteDevice(id);
    setDevices(devices.filter((device) => device.id !== id));
  }

  return (
    <div className="rounded-lg border">
      <Table>
        <TableHeader>
          <TableRow>
            <TableHead>Device</TableHead>
            <TableHead>Status</TableHead>
            <TableHead>Network</TableHead>
            <TableHead>Sensors</TableHead>
            <TableHead>Firmware</TableHead>
            <TableHead className="text-right">Actions</TableHead>
          </TableRow>
        </TableHeader>
        <TableBody>
          {devices.map((device) => {
            const DeviceIcon = getDeviceIcon(device.usage);
            const NetworkIcon = getNetworkIcon(device.network?.kind);
            const { statusVariant, timeAgo } = getStatusInfo(device);
            const { latestTelemetry: telemetry } = device;

            return (
              <TableRow
                key={device.id}
                // using router push because we wrapping <td> or <tr> cannot be a child of <a>
                onClick={() => router.push(`/dashboard/devices/${device.id}`)}
                className="hover:cursor-pointer"
              >
                <TableCell className="font-medium">
                  <div className="flex items-center space-x-2">
                    <DeviceIcon className="text-muted-foreground h-5 w-5" />
                    <div className="w-2"></div>
                    <div className="flex flex-col space-y-1">
                      <div className="flex items-center space-x-2">
                        <span className="font-semibold">{device.label}</span>
                        <Badge variant="outline" className="h-5 px-2 py-0.5 text-xs">
                          {device.usage}
                        </Badge>
                      </div>
                      <div className="flex items-center space-x-1">
                        <span className="text-muted-foreground max-w-[120px] truncate font-mono text-xs">
                          {device.id}
                        </span>
                      </div>
                      {device.lastSeen && (
                        <Tooltip>
                          <TooltipTrigger asChild>
                            <span className="text-muted-foreground cursor-help text-xs">{timeAgo}</span>
                          </TooltipTrigger>
                          <TooltipContent>
                            {/* suppressHydrationWarning is set because SSR and client render tend to produce different results */}
                            <p suppressHydrationWarning>{device.lastSeen.toLocaleString()}</p>
                          </TooltipContent>
                        </Tooltip>
                      )}
                    </div>
                  </div>
                </TableCell>
                <TableCell>
                  <Badge variant={statusVariant}>{device.connected ? 'Online' : 'Offline'}</Badge>
                </TableCell>
                <TableCell className="font-mono text-sm">
                  <div className="flex items-start space-x-2">
                    <NetworkIcon className={`mt-0.5 h-4 w-4 ${device.connected ? 'text-green-500' : 'text-red-500'}`} />
                    <div className="flex flex-col space-y-1 text-sm">
                      <span className="font-mono">{device.network?.localIp || '—'}</span>
                      {device.network?.name && <span className="text-muted-foreground">{device.network?.name}</span>}
                      {device.network?.mac && (
                        <span className="text-muted-foreground font-mono text-xs">{device.network?.mac}</span>
                      )}
                    </div>
                  </div>
                </TableCell>
                <TableCell>
                  <div className="flex flex-col space-y-1 text-sm">
                    {device.usage === 'keeper' ? (
                      <>
                        {telemetry?.temperature && (
                          <span className={getSensorColor(telemetry?.temperature, 'temperature')}>
                            {telemetry?.temperature}°C
                          </span>
                        )}
                        {telemetry?.humidity && (
                          <span className={getSensorColor(telemetry?.humidity, 'humidity')}>
                            {telemetry?.humidity}% RH
                          </span>
                        )}
                      </>
                    ) : (
                      <>
                        {telemetry?.moisture && (
                          <span className={getSensorColor(telemetry?.moisture, 'moisture')}>
                            {telemetry?.moisture.toFixed(1)}% moisture
                          </span>
                        )}
                        {telemetry?.ph && (
                          <span className={getSensorColor(telemetry?.ph, 'ph')}>pH {telemetry?.ph.toFixed(2)}</span>
                        )}
                      </>
                    )}
                    {!telemetry?.temperature && !telemetry?.humidity && !telemetry?.moisture && !telemetry?.ph && (
                      <span className="text-muted-foreground">—</span>
                    )}
                  </div>
                </TableCell>
                <TableCell className="font-mono text-sm">
                  {device.firmware?.currentVersion ? (
                    `v${device.firmware.currentVersion}`
                  ) : (
                    <span className="text-muted-foreground">—</span>
                  )}
                </TableCell>
                <TableCell className="text-right">
                  <div className="flex justify-end space-x-1">
                    <Button variant="ghost" size="sm" asChild>
                      <Link href={`/dashboard/devices/${device.id}/edit`}>
                        <Edit className="h-4 w-4" />
                      </Link>
                    </Button>
                    <AlertDialog>
                      <AlertDialogTrigger asChild>
                        <Button variant="ghost" size="sm">
                          <Trash2 className="h-4 w-4 text-red-500" />
                        </Button>
                      </AlertDialogTrigger>
                      <AlertDialogContent>
                        <AlertDialogHeader>
                          <AlertDialogTitle>Delete Device</AlertDialogTitle>
                          <AlertDialogDescription>
                            {`Are you sure you want to delete device "${device.label}" (${device.id})? This action cannot be undone.`}
                          </AlertDialogDescription>
                        </AlertDialogHeader>
                        <AlertDialogFooter>
                          <AlertDialogCancel>Cancel</AlertDialogCancel>
                          <AlertDialogAction
                            onClick={() => handleDeleteDevice(device.id)}
                            className="bg-red-500 hover:bg-red-600"
                          >
                            Delete
                          </AlertDialogAction>
                        </AlertDialogFooter>
                      </AlertDialogContent>
                    </AlertDialog>
                  </div>
                </TableCell>
              </TableRow>
            );
          })}
        </TableBody>
      </Table>
    </div>
  );
}

function getStatusInfo(device: DisplayableDevice) {
  const lastSeenDate = device.lastSeen ?? new Date(0);
  const now = new Date();
  const diffMs = now.getTime() - lastSeenDate.getTime();
  const diffHours = diffMs / (1000 * 60 * 60);

  let statusVariant: 'default' | 'destructive' | 'secondary' = 'default';
  let timeAgo = '';

  if (!device.connected) {
    if (diffHours > 2) {
      statusVariant = 'destructive'; // red
    } else {
      statusVariant = 'secondary'; // yellow/orange
    }
  }

  // Format time ago
  if (diffMs < 60000) {
    timeAgo = 'now';
  } else if (diffMs < 3600000) {
    timeAgo = `${Math.floor(diffMs / 60000)}m`;
  } else if (diffMs < 86400000) {
    timeAgo = `${Math.floor(diffMs / 3600000)}h`;
  } else {
    timeAgo = `${Math.floor(diffMs / 86400000)}d`;
  }

  return { statusVariant, timeAgo };
}

function getSensorColor(value: number, type: 'moisture' | 'ph' | 'humidity' | 'temperature') {
  switch (type) {
    case 'moisture':
      if (value >= 40) return 'text-blue-600';
      if (value >= 20) return 'text-yellow-600';
      return 'text-red-600';
    case 'ph':
      if (value == -1) return 'text-gray-600'; // when the sensor is not present
      if (value >= 6.0 && value <= 7.5) return 'text-green-600';
      return 'text-yellow-600';
    case 'humidity':
      if (value >= 40 && value <= 70) return 'text-green-600';
      return 'text-yellow-600';
    case 'temperature':
      if (value >= 18 && value <= 28) return 'text-green-600';
      return 'text-yellow-600';
    default:
      return 'text-gray-600';
  }
}
