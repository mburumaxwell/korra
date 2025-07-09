import { notFound } from 'next/navigation';

import { getDevice } from '@/actions';

export const revalidate = 0; // no caching

interface EditDevicePageProps {
  params: Promise<{ id: string }>;
  searchParams: Promise<{ [key: string]: string | string[] | undefined }>;
}

export default async function Page(props: EditDevicePageProps) {
  const params = await props.params;
  const { id } = params;
  const device = await getDevice(id);
  if (!device) {
    notFound();
  }

  return <></>;
}
