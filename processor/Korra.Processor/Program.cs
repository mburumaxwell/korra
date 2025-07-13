using Azure.Identity;
using Korra.Processor;
using Microsoft.Extensions.Configuration.Json;
using Microsoft.Extensions.Configuration.Memory;
using Tingle.EventBus.Configuration;
using SC = Korra.Processor.KorraProcessorSerializerContext;

var builder = Host.CreateApplicationBuilder();

// Not using the appsettings.json file to max portability.
// Add in memory config just after it so that secrets and ENV can override as would be the case if using it normally.
var inMemConfigSource = new MemoryConfigurationSource
{
    InitialData = new Dictionary<string, string?>
    {
        ["Logging:LogLevel:Default"] = "Information",
        ["Logging:LogLevel:Microsoft"] = "Warning",
        ["Logging:LogLevel:Microsoft.Hosting.Lifetime"] = "Information",
        ["Logging:LogLevel:System.Net.Http"] = "Warning",
        ["Logging:LogLevel:Tingle.EventBus"] = "Warning",
        // ["Logging:Debug:LogLevel:Default"] = "None",

        ["Logging:LogLevel:Korra.Processor"] = "Information",
        ["Logging:LogLevel:System.Net.Http.HttpClient"] = "None", // removes all, add what we need later

        ["Logging:Console:FormatterName"] = "cli",
        ["Logging:Console:FormatterOptions:SingleLine"] = "True",
        ["Logging:Console:FormatterOptions:IncludeCategory"] = "False",
        ["Logging:Console:FormatterOptions:IncludeEventId"] = "False",
        ["Logging:Console:FormatterOptions:TimestampFormat"] = "yyyy-MM-dd HH:mm:ss ",

        ["BlobStorage:Endpoint"] = "http://127.0.0.1:10000/devstoreaccount1",
        ["IotHub:EventHubs:ConnectionString"] = "Endpoint=sb://abcd.servicebus.windows.net/;SharedAccessKeyName=xyz;SharedAccessKey=AAAAAAAAAAAAAAAAAAAAAA==",
        ["IotHub:EventHubs:Checkpoints:BlobContainerName"] = "iothub-checkpoints-dev",
        ["IotHub:EventHubs:HubName"] = "iothub-ehub-test-dev-0000000-0aaaa000aa",
        ["Dashboard:Endpoint"] = "http://localhost:3000",
        ["Dashboard:ApiKey"] = "overridden-in-secrets-or-env",
        ["Tinybird:Token"] = "overridden-in-secrets-or-env",
    },
};
var index = builder.Configuration.Sources.IndexOf(
    builder.Configuration.Sources.OfType<JsonConfigurationSource>().Single(cs => cs.Path == "appsettings.json"));
builder.Configuration.Sources.Insert(index + 1, inMemConfigSource);

// Configure logging
builder.Logging.AddCliConsole();

// Add client for sending events to the dashboard
builder.Services.AddHttpApiClient<KorraDashboardClient, KorraDashboardClientOptions>()
                .AddApiKeyHeaderAuthenticationHandler(builder.Configuration["Dashboard:ApiKey"]!, "Bearer")
                .AddUserAgentVersionHandler<KorraDashboardClient>("korra-processor", clear: true)
                .ConfigureHttpClient(client =>
                {
                    client.BaseAddress = new Uri(builder.Configuration["Dashboard:Endpoint"]!, UriKind.Absolute);
                });

// Add client for sending events to Tinybird
// https://www.tinybird.co/docs/api-reference/overview#regions-and-endpoints
builder.Services.Configure<TinybirdClientOptions>(options => options.Token = builder.Configuration["Tinybird:Token"])
                .ConfigureOptions<TinybirdClientConfigureOptions>()
                .AddHttpApiClient<TinybirdClient, TinybirdClientOptions>()
                .AddUserAgentVersionHandler<TinybirdClient>("korra-processor", clear: true)
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
await app.RunAsync();
