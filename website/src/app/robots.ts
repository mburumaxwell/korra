import type { MetadataRoute } from 'next';

import { siteConfig } from '@/site-config';

export default function robots(): MetadataRoute.Robots {
  return {
    rules: [{ userAgent: '*' }],
    sitemap: [`${siteConfig.siteUrl}/sitemap.xml`],
    host: siteConfig.siteUrl,
  };
}
