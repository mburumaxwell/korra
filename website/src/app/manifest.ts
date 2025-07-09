import type { MetadataRoute } from 'next';

import { siteConfig } from '@/site-config';

export default function manifest(): MetadataRoute.Manifest {
  return {
    name: 'Korra',
    short_name: 'Korra',
    start_url: '/',
    background_color: '#F6F9FC',
    theme_color: siteConfig.themeColor,
    display: 'standalone',
    icons: [
      {
        src: '/favicon/android-chrome-192x192.png',
        sizes: '192x192',
        type: 'image/png',
      },
      {
        src: '/favicon/android-chrome-512x512.png',
        sizes: '512x512',
        type: 'image/png',
      },
    ],
  };
}
