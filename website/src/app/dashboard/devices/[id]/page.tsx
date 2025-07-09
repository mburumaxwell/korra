import { notFound } from 'next/navigation';

import { getDevice } from '@/actions';

interface ViewDevicePageProps {
  params: { id: string };
}

export default async function Page({ params }: ViewDevicePageProps) {
  const device = await getDevice(params.id);
  if (!device) {
    notFound();
  }

  return <></>;
}
