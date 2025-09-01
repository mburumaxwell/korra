'use client';

import { Cpu, Download, FileText, Hash, Shield } from 'lucide-react';
import { usePathname, useRouter, useSearchParams } from 'next/navigation';

import { type DisplayableFirmware } from '@/actions';
import { getBoardName, getFrameworkName, getUsageIcon } from '@/components/dashboard/devices';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '@/components/ui/table';
import { TabsContent } from '@/components/ui/tabs';
import { type KorraBoardType, type KorraFirmwareFramework, type KorraUsageType } from '@/lib/schemas';
import { copyToClipboard } from '@/lib/utils';
import { Route } from 'next';

export type AvailableFirmwareTabProps = {
  value: string;
  defaultBoard?: KorraBoardType | 'all';
  defaultUsage?: KorraUsageType | 'all';
  defaultFramework?: KorraFirmwareFramework | 'all';
  entries: DisplayableFirmware[];
};

export function AvailableFirmwareTab({
  value,
  defaultBoard,
  defaultUsage,
  defaultFramework,
  entries,
}: AvailableFirmwareTabProps) {
  const searchParams = useSearchParams();
  const router = useRouter();
  const pathname = usePathname();

  const board = (searchParams.get('board') as KorraBoardType) ?? defaultBoard;
  const usage = (searchParams.get('usage') as KorraUsageType) ?? defaultUsage;
  const framework = (searchParams.get('framework') as KorraFirmwareFramework) ?? defaultFramework;

  function updateParams(updates: {
    board?: KorraBoardType;
    usage?: KorraUsageType;
    framework?: KorraFirmwareFramework;
  }) {
    const params = new URLSearchParams(Array.from(searchParams.entries()));
    if (updates.board != null) params.set('board', updates.board);
    if (updates.usage != null) params.set('usage', updates.usage);
    if (updates.framework != null) params.set('framework', updates.framework);
    router.push((pathname + '?' + params.toString()) as Route);
  }

  return (
    <TabsContent value={value} className="space-y-6">
      <Card>
        <CardHeader>
          <CardTitle>Filter Firmware</CardTitle>
          <CardDescription>Filter available firmware by board type, usage, and framework</CardDescription>
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-1 gap-4 md:grid-cols-3">
            <div className="space-y-2">
              <Label>Board Type</Label>
              <Select value={board} onValueChange={(value: KorraBoardType) => updateParams({ board: value })}>
                <SelectTrigger className="w-full">
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="all">All Boards</SelectItem>
                  <SelectItem value="esp32c6_devkitc">ESP32-C6 DevKitC</SelectItem>
                  <SelectItem value="esp32s3_devkitc">ESP32-S3 DevKitC</SelectItem>
                  <SelectItem value="frdm_rw612">FRDM-RW612</SelectItem>
                  <SelectItem value="nrf7002dk">nRF7002 DK</SelectItem>
                </SelectContent>
              </Select>
            </div>
            <div className="space-y-2">
              <Label>Usage Type</Label>
              <Select value={usage} onValueChange={(value: KorraUsageType) => updateParams({ usage: value })}>
                <SelectTrigger className="w-full">
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="all">All Types</SelectItem>
                  <SelectItem value="keeper">Keeper</SelectItem>
                  <SelectItem value="pot">Pot</SelectItem>
                </SelectContent>
              </Select>
            </div>
            <div className="space-y-2">
              <Label>Framework</Label>
              <Select
                value={framework}
                onValueChange={(value: KorraFirmwareFramework) => updateParams({ framework: value })}
              >
                <SelectTrigger className="w-full">
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="all">All Frameworks</SelectItem>
                  <SelectItem value="arduino">Arduino</SelectItem>
                  <SelectItem value="zephyr">Zephyr</SelectItem>
                  <SelectItem value="espidf">ESP-IDF</SelectItem>
                </SelectContent>
              </Select>
            </div>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>Available Firmware</CardTitle>
          <CardDescription>All firmware versions available for deployment</CardDescription>
        </CardHeader>
        <CardContent>
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Version</TableHead>
                <TableHead>Board</TableHead>
                <TableHead>Usage</TableHead>
                <TableHead>Framework</TableHead>
                <TableHead>Created</TableHead>
                <TableHead>Security</TableHead>
                <TableHead className="text-right">Actions</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {entries.map((firmware) => {
                const UsageIcon = getUsageIcon(firmware.usage);
                return (
                  <TableRow key={firmware.id}>
                    <TableCell className="font-mono font-semibold">{firmware.versionSemver}</TableCell>
                    <TableCell>
                      <div className="flex items-center space-x-2">
                        <Cpu className="text-muted-foreground h-4 w-4" />
                        <span>{getBoardName(firmware.board)}</span>
                      </div>
                    </TableCell>
                    <TableCell>
                      <div className="flex items-center space-x-2">
                        <UsageIcon className="text-muted-foreground h-4 w-4" />
                        <span className="capitalize">{firmware.usage}</span>
                      </div>
                    </TableCell>
                    <TableCell>{getFrameworkName(firmware.framework)}</TableCell>
                    <TableCell className="text-sm" suppressHydrationWarning>
                      {firmware.created.toLocaleString()}
                    </TableCell>
                    <TableCell>
                      <div className="flex items-center space-x-1">
                        <Shield className="h-4 w-4 text-green-500" />
                        <Badge variant="outline" className="text-xs">
                          Signed
                        </Badge>
                      </div>
                    </TableCell>
                    <TableCell className="text-right">
                      <div className="flex justify-end space-x-1">
                        <Button variant="ghost" size="sm" asChild>
                          <a href={firmware.url}>
                            <Download className="h-4 w-4" />
                          </a>
                        </Button>
                        <Button variant="ghost" size="sm" asChild>
                          <a href={firmware.attestation} target="_blank" rel="noopener noreferrer">
                            <FileText className="h-4 w-4" />
                          </a>
                        </Button>
                        <Button variant="ghost" size="sm" onClick={() => copyToClipboard(firmware.hash)}>
                          <Hash className="h-4 w-4" />
                        </Button>
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
