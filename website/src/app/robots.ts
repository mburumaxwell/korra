import type { MetadataRoute } from 'next';

import { config } from '@/site-config';

export default function robots(): MetadataRoute.Robots {
  return {
    rules: [
      { userAgent: '*', allow: '/' },
      // dashboard should not be crawled
      { userAgent: '*', disallow: '/dashboard' },
    ],
    sitemap: [`${config.siteUrl}/sitemap.xml`],
    host: config.siteUrl,
  };
}
