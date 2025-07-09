import type { Metadata, ResolvingMetadata } from 'next';
import { notFound } from 'next/navigation';

type BoardPageProps = {
  params: Promise<{ id: string }>;
  searchParams: Promise<{ [key: string]: string | string[] | undefined }>;
};

export async function generateMetadata(props: BoardPageProps, parent: ResolvingMetadata): Promise<Metadata> {
  // this should be loaded from a markdown engine
  const params = await props.params;
  const { id } = params;
  const board = { id };
  // const board = await getBoard(id);
  if (!board) {
    notFound();
  }

  return {
    title: board.id,
    description: `Support for the ${board.id} board`,
  };
}

export default async function Page(props: BoardPageProps) {
  const params = await props.params;
  const { id } = params;
  const board = { id };
  // const board = await getBoard(id);
  if (!board) {
    notFound();
  }

  return <></>;
}
