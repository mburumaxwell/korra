import { getSiteUrl } from '@/lib/environment';

const siteUrl = getSiteUrl({ defaultValue: 'https://korra.maxwellweru.com' });

export const config = {
  siteUrl: siteUrl.replace(/\/$/, ''),
  themeColor: '#264C3F',
  title: 'Korra',
  description: 'Korra is my little IoT learning project.',

  dashboard: {
    title: 'Korra Dashboard',
    description: 'Management dashboard for Korra',
  },
};

// just me, because this is a toy/showcase, no need for dedicated handles
export const socials = {
  twitter: {
    username: 'maxwellweru',
    url: 'https://twitter.com/maxwellweru',
  },
  linkedin: {
    username: 'maxwellweru',
    url: 'https://www.linkedin.com/in/maxwellweru',
  },
  youtube: {
    channel: 'mburumaxwell',
    url: 'https://youtube.com/c/mburumaxwell',
  },
  github: {
    username: 'mburumaxwell',
    url: 'https://github.com/mburumaxwell',
  },
};
