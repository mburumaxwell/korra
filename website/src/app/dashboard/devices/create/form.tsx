'use client';

import { useRouter } from 'next/navigation';
import { useState, useTransition } from 'react';

import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Textarea } from '@/components/ui/textarea';
import { type KorraUsageType } from '@/lib/schemas';

export function CreateDeviceForm() {
  const router = useRouter();
  const [isPending, startTransition] = useTransition();
  const [device, setDevice] = useState({
    label: '',
    usage: 'pot' as KorraUsageType,
    certificatePem: '',
  });

  function handleSubmit(e: React.FormEvent) {
    e.preventDefault();
    if (device.label && device.usage) {
      startTransition(async () => {
        // await createDevice({
        //   label: device.label,
        //   usage: device.usage,
        //   certificatePem: device.certificatePem,
        // });
        router.push('/dashboard/devices');
      });
    }
  }

  return (
    <form onSubmit={handleSubmit} className="space-y-6">
      <div className="grid gap-4">
        <div className="grid gap-2">
          <Label htmlFor="usage">Usage</Label>
          <Select
            value={device.usage}
            onValueChange={(value: 'keeper' | 'pot') => setDevice({ ...device, usage: value })}
          >
            <SelectTrigger>
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="keeper">Keeper</SelectItem>
              <SelectItem value="pot">Pot</SelectItem>
            </SelectContent>
          </Select>
        </div>

        <div className="grid gap-2">
          <Label htmlFor="label">Device Label</Label>
          <Input
            id="label"
            value={device.label}
            onChange={(e) => setDevice({ ...device, label: e.target.value })}
            placeholder={device.usage === 'keeper' ? 'K1' : 'W1, D2, M3'}
            required
          />
        </div>

        <div className="grid gap-2">
          <Label htmlFor="certificatePem">Device Certificate (PEM)</Label>
          <Textarea
            id="certificatePem"
            value={device.certificatePem}
            rows={10}
            onChange={(e) => setDevice({ ...device, certificatePem: e.target.value })}
            placeholder={`-----BEGIN CERTIFICATE-----
MIIBHjCBxaADAgECAgEBMAoGCCqGSM49BAMCMBcxFTATBgNVBAMMDDVjN2Y4NmZh
MTJmNDAeFw0yNDEyMzEyMzU5NTlaFw0yNzEyMzEyMzU5NTlaMBcxFTATBgNVBAMM
DDVjN2Y4NmZhMTJmNDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABHVEysshzymv
jU1iVY/au1XtVm1ESEGtn88jFLqAeDldmZT11mmdIHT3lUdu++0nFmj4ZDmXJIV+
65uYF3Q6yYGjAjAAMAoGCCqGSM49BAMCA0gAMEUCIEsr5D0CnxlWZg4lCd+uXl1b
fMs3gxoO8wLxRCqlj1reAiEA5DP8j23TVuN6Rh3lDkozJJFLJDF8Sy1oAOgyQAQ8
oio=
-----END CERTIFICATE-----`}
          />
        </div>
      </div>

      <div className="flex justify-end space-x-2">
        <Button type="submit" disabled={isPending}>
          {isPending ? 'Creating...' : 'Create Device'}
        </Button>
      </div>
    </form>
  );
}
