import { Analytics } from '@vercel/analytics/react';
import { SpeedInsights } from '@vercel/speed-insights/next';
import type { Metadata, Viewport } from 'next';
import type { TemplateString } from 'next/dist/lib/metadata/types/metadata-types';

import { Provider } from '@/components';
import '../../globals.css';

import { config } from '@/site-config';

const titleTemplate: TemplateString = {
  default: config.dashboard.title,
  template: `%s | ${config.dashboard.title}`,
};

export const metadata: Metadata = {
  title: titleTemplate,
  description: config.dashboard.description,
  metadataBase: new URL(`${config.siteUrl}/dashboard`),
  openGraph: {
    type: 'website',
    title: titleTemplate,
    description: config.dashboard.description,
    url: `${config.siteUrl}/dashboard`,
  },
  robots: {
    index: false,
    follow: false,
    googleBot: {
      index: false,
      follow: false,
      noarchive: true,
    },
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
