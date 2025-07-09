'use client';

import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, type XAxisProps, YAxis } from 'recharts';

import type { BucketedDeviceTelemetry } from '@/actions';

export type TelemetryChartConfigKey = keyof Omit<BucketedDeviceTelemetry, 'bucket'>;

export type TelemetryChartConfig = {
  key: TelemetryChartConfigKey;
  title: string;
  domain: [number | string, number | string];
  decimals: number;
  suffix: string;
};

export type TelemetryChartProps = {
  data: BucketedDeviceTelemetry[];
  tickFormatter?: XAxisProps['tickFormatter'];
  config: TelemetryChartConfig;
};

export function TelemetryChart({
  data,
  tickFormatter,
  config: { key, title, domain, decimals, suffix },
}: TelemetryChartProps) {
  return (
    <div>
      <h4 className="text-lg font-medium mb-2">{title}</h4>
      <div className="h-[300px]">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="bucket" tickFormatter={tickFormatter} scale="time" />
            <YAxis domain={domain} tickCount={10} />
            <Tooltip
              labelFormatter={(value: Date) => value.toLocaleString()}
              formatter={(value: number) => [`${value.toFixed(decimals)}${suffix}`, title]}
            />
            <Line type="monotone" dataKey={key} strokeWidth={2} dot={false} name={title} />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
