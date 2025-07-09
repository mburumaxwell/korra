using Azure.Identity;
using Korra.Processor;
using Tingle.EventBus.Configuration;
using SC = Korra.Processor.KorraProcessorSerializerContext;

var builder = Host.CreateApplicationBuilder();

// Add client for sending events to the dashboard
builder.Services.AddHttpApiClient<KorraDashboardClient, KorraDashboardClientOptions>()
                .AddApiKeyHeaderAuthenticationHandler(builder.Configuration["Dashboard:ApiKey"]!, "Bearer")
                .ConfigureHttpClient(client =>
                {
                    client.BaseAddress = new Uri(builder.Configuration["Dashboard:Endpoint"]!, UriKind.Absolute);
                });

// Add client for sending events to Tinybird
// https://www.tinybird.co/docs/api-reference/overview#regions-and-endpoints
builder.Services.Configure<TinybirdClientOptions>(options => options.Token = builder.Configuration["Tinybird:Token"])
                .ConfigureOptions<TinybirdClientConfigureOptions>()
                .AddHttpApiClient<TinybirdClient, TinybirdClientOptions>()
                .ConfigureHttpClient(client => client.BaseAddress = new Uri("https://api.europe-west2.gcp.tinybird.co"));

builder.Services.AddSlimEventBus(eb =>
{
    eb.ConfigureSerialization(options => options.SerializerOptions.TypeInfoResolverChain.Insert(0, SC.Default));

    eb.Configure(o => o.ConfigureEvent<KorraIotHubEvent>(reg =>
    {
        reg.ConfigureAsIotHubEvent(builder.Configuration["IotHub:EventHubs:HubName"]!)
           .UseIotHubEventSerializer();
    }));

    eb.AddConsumer<KorraIotHubEvent, KorraIotEventsConsumer>();

    // Transport specific configuration
    eb.AddAzureEventHubsTransport(options =>
    {
        options.Credentials = builder.Configuration["IotHub:EventHubs:ConnectionString"]!;
        var blobUrl = builder.Configuration["BlobStorage:Endpoint"]!;
        var isLocal = !blobUrl.Contains(".core.windows.net");
        options.BlobStorageCredentials = !isLocal
            ? new AzureBlobStorageCredentials { ServiceUrl = new Uri(blobUrl), TokenCredential = new DefaultAzureCredential(), }
            : "UseDevelopmentStorage=true";
        options.BlobContainerName = builder.Configuration["IotHub:EventHubs:Checkpoints:BlobContainerName"]!;
    });
});

var app = builder.Build();

// run continuously or for a given duration, if provided
if (int.TryParse(builder.Configuration["JOB_DURATION_SECONDS"], out var jobDurationSeconds))
{
    var cts = new CancellationTokenSource(TimeSpan.FromSeconds(jobDurationSeconds));
    await app.RunAsync(cts.Token);
}
else await app.RunAsync();
