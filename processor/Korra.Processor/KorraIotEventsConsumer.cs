using System.Text.Json;
using Azure.Messaging.EventHubs.Consumer;
using Tingle.EventBus;
using Tingle.EventBus.Configuration;
using Tingle.EventBus.Transports.Azure.EventHubs.IotHub;
using SC = Korra.Processor.KorraProcessorSerializerContext;

namespace Korra.Processor;

[ConsumerName(EventHubConsumerClient.DefaultConsumerGroupName)]
internal class KorraIotEventsConsumer(KorraDashboardClient dashboardClient,
                                      TinybirdClient tinybirdClient,
                                      ILogger<KorraIotEventsConsumer> logger) : IEventConsumer<KorraIotHubEvent>
{
    public async Task ConsumeAsync(EventContext<KorraIotHubEvent> context, CancellationToken cancellationToken)
    {
        var evt = context.Event;
        switch (evt.Source)
        {
            case IotHubEventMessageSource.Telemetry:
                {
                    var telemetry = evt.GetTelemetry(SC.Default.KorraIotHubTelemetry);
                    await HandleTelemetryAsync(context, telemetry, cancellationToken);
                    break;
                }
            case IotHubEventMessageSource.TwinChangeEvents:
            case IotHubEventMessageSource.DeviceLifecycleEvents:
            case IotHubEventMessageSource.DeviceConnectionStateEvents:
                {
                    await HandleOperationalEventAsync(context, evt.Source, evt.Event!, cancellationToken);
                    break;
                }
        }
    }

    internal virtual async Task HandleTelemetryAsync(EventContext context,
                                                     KorraIotHubTelemetry incoming,
                                                     CancellationToken cancellationToken = default)
    {
        var deviceId = context.GetIotHubDeviceId() ?? throw new InvalidOperationException("device id cannot be null");
        var enqueued = context.GetIotHubEnqueuedTime();

        var appKind = incoming.AppKind switch
        {
            KorraIotHubTelemetryAppKind.Keeper => KorraAppKind.Keeper,
            KorraIotHubTelemetryAppKind.Pot => KorraAppKind.Pot,
            null => incoming.PH is not null ? KorraAppKind.Pot : KorraAppKind.Keeper,
            _ => throw new NotImplementedException(),
        };
        // var telemetryId = Tingle.Extensions.Primitives.Ksuid.Generate(incoming.Created);
        var telemetryId = $"{context.GetEventData().SequenceNumber}";

        var telemetry = new KorraTelemetry
        {
            Id = telemetryId,
            DeviceId = deviceId,
            Created = incoming.Created,
            Received = enqueued?.ToUniversalTime(),
            AppKind = appKind,
            // we assume the units for this do not vary, otherwise we would need to convert
            Temperature = incoming.Temperature?.Value,
            Humidity = incoming.Humidity?.Value,
            Moisture = incoming.Moisture?.Value,
            PH = incoming.PH?.Value,
        };

        if (logger.IsEnabled(LogLevel.Debug))
        {
            logger.LogDebug("Forwarding Telemetry from {DeviceId}\r\n{Telemetry}",
                            deviceId,
                            JsonSerializer.Serialize(telemetry, SC.Default.KorraTelemetry));
        }
        await dashboardClient.SendTelemetryAsync(telemetry, cancellationToken);

        var tinybirdPayload = new System.Text.Json.Nodes.JsonObject
        {
            ["id"] = telemetry.Id,
            // tinybird expects a specific format for date-time
            ["timestamp"] = telemetry.Created.ToString("yyyy-MM-dd'T'HH:mm:ss"),
            ["device_id"] = telemetry.DeviceId,
            ["app_kind"] = telemetry.AppKind.GetEnumMemberAttrValueOrDefault(),
            ["temperature"] = telemetry.Temperature,
            ["humidity"] = telemetry.Humidity,
            ["moisture"] = telemetry.Moisture,
            ["ph"] = telemetry.PH,
        };
        await tinybirdClient.SendAsync("telemetry", tinybirdPayload, cancellationToken);
    }

    internal virtual async Task HandleOperationalEventAsync(EventContext context,
                                                            IotHubEventMessageSource source,
                                                            IotHubOperationalEvent ope,
                                                            CancellationToken cancellationToken = default)
    {
        KorraOperationalEventType? type = ope.Type switch
        {
            IotHubOperationalEventType.UpdateTwin => KorraOperationalEventType.TwinUpdated,
            IotHubOperationalEventType.ReplaceTwin => KorraOperationalEventType.TwinUpdated,
            IotHubOperationalEventType.DeviceDisconnected => KorraOperationalEventType.Disconnected,
            IotHubOperationalEventType.DeviceConnected => KorraOperationalEventType.Connected,
            _ => null,
        };
        if (type is null) return;

        var @event = new KorraOperationalEvent
        {
            Type = type.Value,
            DeviceId = ope.DeviceId,
            SequenceNumber = ope.Payload.SequenceNumber,
        };

        if (logger.IsEnabled(LogLevel.Debug))
        {
            logger.LogDebug("Forwarding Operational Event for {DeviceId}\r\n{Event}",
                            ope.DeviceId,
                            JsonSerializer.Serialize(@event, SC.Default.KorraOperationalEvent));
        }
        await dashboardClient.SendOperationalEventAsync(@event, cancellationToken);
    }
}
