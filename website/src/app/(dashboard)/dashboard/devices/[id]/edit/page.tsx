import { notFound } from 'next/navigation';

import { getDevice } from '@/actions';

export const revalidate = 0; // no caching

export default async function Page(props: PageProps<'/dashboard/devices/[id]/edit'>) {
  const params = await props.params;
  const { id } = params;
  const device = await getDevice(id);
  if (!device) {
    notFound();
  }

  return <></>;
}
