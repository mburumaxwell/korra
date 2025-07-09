import { notFound } from 'next/navigation';

import { getDevice } from '@/actions';

interface EditDevicePageProps {
  params: { id: string };
}

export default async function Page({ params }: EditDevicePageProps) {
  const device = await getDevice(params.id);
  if (!device) {
    notFound();
  }

  return <></>;
}
