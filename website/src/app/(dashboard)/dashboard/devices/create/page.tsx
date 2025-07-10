import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { CreateDeviceForm } from './form';

export default async function Page() {
  return (
    <div className="container mx-auto max-w-2xl p-6">
      <div className="mb-6 flex items-center space-x-4">
        <h1 className="text-3xl font-bold">Create New Device</h1>
      </div>

      <Card>
        <CardHeader>
          <CardTitle>Device Information</CardTitle>
        </CardHeader>
        <CardContent>
          <CreateDeviceForm />
        </CardContent>
      </Card>
    </div>
  );
}
