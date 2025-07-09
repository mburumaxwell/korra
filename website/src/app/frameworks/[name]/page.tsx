import type { Metadata, ResolvingMetadata } from 'next';
import { notFound } from 'next/navigation';

type FrameworkPageProps = {
  params: Promise<{ name: string }>;
  searchParams: Promise<{ [key: string]: string | string[] | undefined }>;
};

export async function generateMetadata(props: FrameworkPageProps, parent: ResolvingMetadata): Promise<Metadata> {
  // this should be loaded from a markdown engine
  const params = await props.params;
  const { name } = params;
  const framework = { name };
  // const framework = await getFramework(id);
  if (!framework) {
    notFound();
  }

  return {
    title: framework.name,
    description: `Support for the ${framework.name} framework`,
  };
}

export default async function Page(props: FrameworkPageProps) {
  const params = await props.params;
  const { name } = params;
  const framework = { name };
  // const framework = await getFramework(id);
  if (!framework) {
    notFound();
  }

  return <></>;
}
