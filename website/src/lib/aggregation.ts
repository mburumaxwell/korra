export type TimeRange = '1h' | '4h' | '6h' | '24h' | '7d';
export type Granularity = '5m' | '15m' | '30m' | '1h' | '6h' | '12h' | '1d';

export const timeRangeOptions: { value: TimeRange; label: string }[] = [
  { value: '1h', label: 'Last 1 Hour' },
  { value: '4h', label: 'Last 4 Hours' },
  { value: '6h', label: 'Last 6 Hours' },
  { value: '24h', label: 'Last 24 Hours' },
  { value: '7d', label: 'Last 7 Days' },
];

export const granularityOptions: { value: Granularity; label: string }[] = [
  { value: '5m', label: '5 Minutes' },
  { value: '15m', label: '15 Minutes' },
  { value: '30m', label: '30 Minutes' },
  { value: '1h', label: '1 Hour' },
  { value: '6h', label: '6 Hours' },
  { value: '12h', label: '12 Hours' },
  { value: '1d', label: '1 Day' },
];

export function getDateTimeRange(value: TimeRange): { start: Date; end: Date } {
  const end = new Date();
  switch (value) {
    case '1h':
      return { start: new Date(end.getTime() - 1 * 60 * 60 * 1000), end };
    case '4h':
      return { start: new Date(end.getTime() - 4 * 60 * 60 * 1000), end };
    case '6h':
      return { start: new Date(end.getTime() - 6 * 60 * 60 * 1000), end };
    case '24h':
      return { start: new Date(end.getTime() - 24 * 60 * 60 * 1000), end };
    case '7d':
      return { start: new Date(end.getTime() - 7 * 24 * 60 * 60 * 1000), end };
  }
}

export function granularityToMilliseconds(value: Granularity): number {
  const map: Record<Granularity, number> = {
    '5m': 5 * 60 * 1_000,
    '15m': 15 * 60 * 1_000,
    '30m': 30 * 60 * 1_000,
    '1h': 1 * 60 * 60 * 1_000,
    '6h': 6 * 60 * 60 * 1_000,
    '12h': 12 * 60 * 60 * 1_000,
    '1d': 24 * 60 * 60 * 1_000,
  };
  const ms = map[value];
  if (!ms) throw new Error(`Unsupported granularity: ${value}`);
  return ms;
}
