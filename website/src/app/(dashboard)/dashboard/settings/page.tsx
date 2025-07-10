import { type Metadata } from 'next';

export const revalidate = 0; // no caching

export const metadata: Metadata = {
  title: 'Settings',
  description: 'Configure system settings and preferences',
};

export default function Page() {
  return (
    <div className="container mx-auto p-6 pt-16 lg:pt-6">
      <div className="space-y-6">
        <div>
          <h1 className="text-3xl font-bold">Settings</h1>
          <p className="text-muted-foreground">Configure system settings and preferences</p>
        </div>

        <div className="rounded-lg border p-8 text-center">
          <p className="text-muted-foreground">Settings panel coming soon...</p>
        </div>
      </div>
    </div>
  );
}
