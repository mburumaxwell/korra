import { Analytics } from '@vercel/analytics/react';
import { SpeedInsights } from '@vercel/speed-insights/next';
import type { Metadata, Viewport } from 'next';
import type { TemplateString } from 'next/dist/lib/metadata/types/metadata-types';

import { Provider } from '@/components';
import './globals.css';

import { config } from '@/site-config';

const titleTemplate: TemplateString = {
  default: config.title,
  template: `%s | ${config.title}`,
};

export const metadata: Metadata = {
  title: titleTemplate,
  description: config.description,
  metadataBase: new URL(config.siteUrl),
  openGraph: {
    type: 'website',
    title: titleTemplate,
    description: config.description,
    url: config.siteUrl,
  },
};

export const viewport: Viewport = {
  themeColor: config.themeColor,
  width: 'device-width',
  initialScale: 1,
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en">
      <body>
        <Provider>{children}</Provider>
        <Analytics />
        <SpeedInsights />
      </body>
    </html>
  );
}
