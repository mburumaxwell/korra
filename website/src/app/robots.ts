import type { MetadataRoute } from 'next';

import { config } from '@/site-config';

export default function robots(): MetadataRoute.Robots {
  return {
    rules: [{ userAgent: '*' }],
    sitemap: [`${config.siteUrl}/sitemap.xml`],
    host: config.siteUrl,
  };
}
