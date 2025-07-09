import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { CreateDeviceForm } from './form';

export default async function Page() {
  return (
    <div className="container mx-auto p-6 max-w-2xl">
      <div className="flex items-center space-x-4 mb-6">
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
