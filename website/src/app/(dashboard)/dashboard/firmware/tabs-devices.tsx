'use client';

import { AlertCircle, CheckCircle, Clock, RefreshCw, Zap } from 'lucide-react';

import { type DisplayableDevice, type DisplayableFirmware } from '@/actions';
import { getUsageIcon } from '@/components/dashboard/devices';
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
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '@/components/ui/table';
import { TabsContent } from '@/components/ui/tabs';
import { KorraBoardType, KorraFirmwareFramework, KorraUsageType } from '@/lib/schemas';

export type DevicesFirmwareTabProps = {
  value: string;
  devices: DisplayableDevice[];
  latestFirmwareEntries: DisplayableFirmware[];
};

export function DevicesFirmwareTab({ value, devices, latestFirmwareEntries }: DevicesFirmwareTabProps) {
  const getFirmwareKey = (board: KorraBoardType, usage: KorraUsageType, framework: KorraFirmwareFramework) =>
    `${board}|${usage}|${framework}`;

  const firmwareMap = new Map<string, DisplayableFirmware>();
  latestFirmwareEntries.forEach((firmware) => {
    const key = getFirmwareKey(firmware.board, firmware.usage, firmware.framework);
    firmwareMap.set(key, firmware);
  });

  type DeviceFirmwareStatus = 'updating' | 'pending' | 'up-to-date' | 'unknown';
  function getDeviceLatestFirmwareStatus(device: DisplayableDevice): DeviceFirmwareStatus {
    const { firmware, board, usage, framework } = device;

    if (!framework) return 'unknown';
    if (!firmware?.currentVersion || !firmware.desiredVersion) return 'unknown';

    const latest = firmwareMap.get(getFirmwareKey(board, usage, framework));
    if (!latest) return 'unknown';

    const current = firmware.currentVersion;
    const desired = firmware.desiredVersion;

    if (current !== latest.versionSemver && latest.versionSemver !== desired) return 'pending';
    if (current === desired) return 'up-to-date';
    return 'updating';
  }

  function getStatusIcon(status: DeviceFirmwareStatus) {
    switch (status) {
      case 'up-to-date':
        return <CheckCircle className="h-4 w-4 text-green-500" />;
      case 'updating':
        return <RefreshCw className="h-4 w-4 animate-spin text-blue-500" />;
      case 'pending':
        return <Clock className="h-4 w-4 text-yellow-500" />;
      // case "failed":
      //   return <AlertCircle className="h-4 w-4 text-red-500" />;
      default:
        return <AlertCircle className="h-4 w-4 text-gray-500" />;
    }
  }

  function getStatusLabel(status: DeviceFirmwareStatus) {
    switch (status) {
      case 'up-to-date':
        return 'Up to Date';
      case 'updating':
        return 'Updating';
      case 'pending':
        return 'Pending';
      // case "failed":
      //   return 'Failed';
      default:
        return 'Unknown';
    }
  }

  return (
    <TabsContent value={value} className="space-y-6">
      <div className="grid grid-cols-1 gap-4 md:grid-cols-4 lg:grid-cols-4">
        <Card>
          <CardContent>
            <div className="flex items-center space-x-2">
              <CheckCircle className="h-5 w-5 text-green-500" />
              <div>
                <p className="text-2xl font-bold">
                  {devices.filter((d) => getDeviceLatestFirmwareStatus(d) === 'up-to-date').length}
                </p>
                <p className="text-muted-foreground text-xs">Up to Date</p>
              </div>
            </div>
          </CardContent>
        </Card>
        <Card>
          <CardContent>
            <div className="flex items-center space-x-2">
              <RefreshCw className="h-5 w-5 text-blue-500" />
              <div>
                <p className="text-2xl font-bold">
                  {devices.filter((d) => getDeviceLatestFirmwareStatus(d) === 'updating').length}
                </p>
                <p className="text-muted-foreground text-xs">Updating</p>
              </div>
            </div>
          </CardContent>
        </Card>
        <Card>
          <CardContent>
            <div className="flex items-center space-x-2">
              <Clock className="h-5 w-5 text-yellow-500" />
              <div>
                <p className="text-2xl font-bold">
                  {devices.filter((d) => getDeviceLatestFirmwareStatus(d) === 'pending').length}
                </p>
                <p className="text-muted-foreground text-xs">Pending</p>
              </div>
            </div>
          </CardContent>
        </Card>
        <Card>
          <CardContent>
            <div className="flex items-center space-x-2">
              <Zap className="h-5 w-5 text-green-600" />
              <div>
                <p className="text-2xl font-bold">{devices.filter((d) => d.connected).length}</p>
                <p className="text-muted-foreground text-xs">Online</p>
              </div>
            </div>
          </CardContent>
        </Card>
      </div>
      <Card>
        <CardHeader>
          <CardTitle>Device Firmware Status</CardTitle>
          <CardDescription>Current firmware versions and update status for all devices</CardDescription>
        </CardHeader>
        <CardContent>
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Device</TableHead>
                <TableHead>Current Version</TableHead>
                <TableHead>Desired Version</TableHead>
                <TableHead>Status</TableHead>
                <TableHead>Last Seen</TableHead>
                <TableHead className="text-right">Actions</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {devices.map((device) => {
                const UsageIcon = getUsageIcon(device.usage);
                const status = getDeviceLatestFirmwareStatus(device);

                return (
                  <TableRow key={device.id}>
                    <TableCell className="font-medium">
                      <div className="flex items-center space-x-2">
                        <UsageIcon className="text-muted-foreground h-4 w-4" />
                        <div>
                          <div className="font-semibold">{device.label}</div>
                          <div className="text-muted-foreground text-xs capitalize">
                            {device.usage} • {device.framework}
                          </div>
                        </div>
                      </div>
                    </TableCell>
                    <TableCell className="font-mono text-sm">{device.firmware?.currentVersion}</TableCell>
                    <TableCell className="font-mono text-sm">{device.firmware?.desiredVersion}</TableCell>
                    <TableCell>
                      <div className="flex items-center space-x-2">
                        {getStatusIcon(status)}
                        <span>{getStatusLabel(status)}</span>
                      </div>
                    </TableCell>
                    <TableCell className="text-sm" suppressHydrationWarning>
                      {device.lastSeen?.toLocaleString()}
                    </TableCell>
                    <TableCell className="text-right">
                      <div className="flex justify-end space-x-1">
                        <Button variant="ghost" size="sm" disabled={!device.connected}>
                          <RefreshCw className="h-4 w-4" />
                        </Button>
                        <AlertDialog>
                          <AlertDialogTrigger asChild>
                            <Button variant="ghost" size="sm" disabled={!device.connected}>
                              <Zap className="h-4 w-4" />
                            </Button>
                          </AlertDialogTrigger>
                          <AlertDialogContent>
                            <AlertDialogHeader>
                              <AlertDialogTitle>Force Firmware Update</AlertDialogTitle>
                              <AlertDialogDescription>
                                This will immediately trigger a firmware update for device &quot;{device.label}&quot;.
                                The device must be online and this action cannot be undone.
                              </AlertDialogDescription>
                            </AlertDialogHeader>
                            <AlertDialogFooter>
                              <AlertDialogCancel>Cancel</AlertDialogCancel>
                              <AlertDialogAction>Force Update</AlertDialogAction>
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
        </CardContent>
      </Card>
    </TabsContent>
  );
}
