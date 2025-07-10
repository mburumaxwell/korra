import { type Metadata } from 'next';

export const revalidate = 60; // cache for 60 seconds

export const metadata: Metadata = {
  title: 'Analytics',
  description: 'View system analytics and insights',
};

export default function Page() {
  return (
    <div className="container">
      <div className="space-y-6">
        <div>
          <h1 className="text-3xl font-bold">Analytics</h1>
          <p className="text-muted-foreground">View system analytics and insights</p>
        </div>

        <div className="rounded-lg border p-8 text-center">
          <p className="text-muted-foreground">Analytics dashboard coming soon...</p>
        </div>
      </div>
    </div>
  );
}
