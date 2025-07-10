import { Analytics } from '@vercel/analytics/react';
import { SpeedInsights } from '@vercel/speed-insights/next';
import type { Metadata, Viewport } from 'next';
import type { TemplateString } from 'next/dist/lib/metadata/types/metadata-types';

import { Provider } from '@/components/provider';
import '../../globals.css';

import { Navigation } from '@/components/dashboard/navigation';
import { environment } from '@/lib/environment';
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
        <Provider>
          <Navigation environment={environment} />
          <main className="lg:pl-64">
            {/* <div className="container mx-auto p-6 pt-16 lg:pt-6 space-y-6">{children}</div> */}
            <div className="mx-auto space-y-6 p-6 pt-16 lg:pt-6">{children}</div>
          </main>
        </Provider>
        <Analytics />
        <SpeedInsights />
      </body>
    </html>
  );
}
