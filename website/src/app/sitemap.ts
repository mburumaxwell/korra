import type { MetadataRoute } from 'next';

import { config } from '@/site-config';

export default function sitemap(): MetadataRoute.Sitemap {
  type Route = MetadataRoute.Sitemap[number];

  const routes = [
    '', // root without trailing slash
    '/contact',
  ].map(
    (route): Route => ({
      url: `${config.siteUrl}${route}`,
      // lastModified: new Date().toISOString(),
      changeFrequency: 'daily',
      priority: 0.5,
    }),
  );

  return routes;
}
