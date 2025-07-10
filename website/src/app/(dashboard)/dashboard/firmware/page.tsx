import { type Metadata } from 'next';

export const revalidate = 60; // cache for 60 seconds

export const metadata: Metadata = {
  title: 'Firmware',
  description: 'Manage firmware versions',
};

export default function Page() {
  return (
    <div className="container">
      <div className="space-y-6">
        <div>
          <h1 className="text-3xl font-bold">Firmware</h1>
          <p className="text-muted-foreground">Manage firmware versions and updates</p>
        </div>

        <div className="rounded-lg border p-8 text-center">
          <p className="text-muted-foreground">Firmware management coming soon...</p>
        </div>
      </div>
    </div>
  );
}
