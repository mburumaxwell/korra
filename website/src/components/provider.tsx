'use client';

import type { ReactNode } from 'react';

import { ApplicationInsightsProvider } from './app-insights';

export function Provider({ children }: { children: ReactNode }) {
  return (
    <ApplicationInsightsProvider connectionString={process.env.NEXT_PUBLIC_APP_INSIGHTS_CONNECTION_STRING}>
      {children}
    </ApplicationInsightsProvider>
  );
}
