import { getSiteUrl } from '@/lib/environment';

const siteUrl = getSiteUrl({ defaultValue: 'https://korra.maxwellweru.com' });

export const siteConfig = {
  siteUrl: siteUrl.replace(/\/$/, ''),
  themeColor: '#264C3F',
  title: 'Korra',
  description: 'Korra is my little IoT learning project.',

  dashboard: {
    title: 'Korra Dashboard',
    description: 'Management dashboard for Korra',
  },
};
