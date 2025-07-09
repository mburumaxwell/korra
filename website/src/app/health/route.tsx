// This is is dedicated endpoint/route for health checks.
// It is used by monitoring tools to check the health of the application.

export async function GET() {
  const content: 'Healthy' | 'Unhealthy' = 'Healthy'; // default to healthy

  // you can check for database connections here if in use

  return new Response(content, {
    status: 200,
    headers: {
      'Content-Type': 'text/plain',
    },
  });
}
