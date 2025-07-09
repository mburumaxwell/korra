import { getDevices, type DisplayableDevice } from '@/actions';
import { Badge } from '@/components/ui/badge';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '@/components/ui/table';
import { Trash2, Plus, Edit, Wifi, WifiOff, Flower2, Settings } from 'lucide-react';

export default async function Page() {
  const devices = await getDevices();

  return (
    <div className="container mx-auto p-6 space-y-6">
      <div className="flex justify-between items-center">
        <div>
          <h1 className="text-3xl font-bold">Korra IoT Manager</h1>
          <p className="text-muted-foreground">
            Managing {devices.filter((d) => d.usage === 'keeper').length} keeper(s) and{' '}
            {devices.filter((d) => d.usage === 'pot').length} pot(s)
          </p>
        </div>
        {/* TODO: could place add dialog here */}
      </div>

      <div className="border rounded-lg">
        <Table>
          <TableHeader>
            <TableRow>
              <TableHead>Device</TableHead>
              <TableHead>Usage</TableHead>
              <TableHead>Status</TableHead>
              <TableHead>Network</TableHead>
              <TableHead>Sensors</TableHead>
              <TableHead>Firmware</TableHead>
              <TableHead className="text-right">Actions</TableHead>
            </TableRow>
          </TableHeader>
          <TableBody>
            {devices.map((device) => {
              const DeviceIcon = device.usage === 'keeper' ? Settings : Flower2;
              const { statusColor, statusVariant, timeAgo } = getStatusInfo(device);
              const { latestTelemetry: telemetry } = device;

              return (
                <TableRow key={device.id}>
                  <TableCell className="font-medium">
                    <div className="flex items-center space-x-2">
                      <DeviceIcon className="w-4 h-4 text-muted-foreground" />
                      <div className="flex flex-col">
                        <span className="font-semibold">{device.label}</span>
                        <span className="text-xs text-muted-foreground">{timeAgo}</span>
                      </div>
                      {device.connected ? (
                        <Wifi className="w-3 h-3 text-green-500" />
                      ) : (
                        <WifiOff className="w-3 h-3 text-red-500" />
                      )}
                    </div>
                  </TableCell>
                  <TableCell>
                    <Badge variant="outline" className="capitalize">
                      {device.usage}
                    </Badge>
                  </TableCell>
                  <TableCell>
                    <Badge variant={statusVariant} className={`${statusColor} text-white`}>
                      {device.connected ? 'Online' : 'Offline'}
                    </Badge>
                  </TableCell>
                  <TableCell className="font-mono text-sm">
                    {/* {device.network?.local_ip || <span className="text-muted-foreground">—</span>} */}
                    {/* {device.network?.name || <span className="text-muted-foreground">—</span>} */}
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
                              {telemetry?.moisture}% moisture
                            </span>
                          )}
                          {telemetry?.ph && (
                            <span className={getSensorColor(telemetry?.ph, 'ph')}>pH {telemetry?.ph}</span>
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
                      {/* dialog with triggers for edit and delete? */}
                    </div>
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

function getStatusInfo(device: DisplayableDevice) {
  const lastSeenDate = device.lastSeen ?? new Date(0);
  const now = new Date();
  const diffMs = now.getTime() - lastSeenDate.getTime();
  const diffHours = diffMs / (1000 * 60 * 60);

  let statusColor = 'bg-green-500'; // online - green
  let statusVariant: 'default' | 'destructive' | 'secondary' = 'default';
  let timeAgo = '';

  if (!device.connected) {
    if (diffHours > 2) {
      statusColor = 'bg-red-500'; // offline > 2 hours - red
      statusVariant = 'destructive';
    } else {
      statusColor = 'bg-yellow-500'; // offline < 2 hours - yellow/orange
      statusVariant = 'secondary';
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

  return { statusColor, statusVariant, timeAgo };
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
