using System.Text.Json;
using Azure.Messaging.EventHubs.Consumer;
using Tingle.EventBus;
using Tingle.EventBus.Configuration;
using Tingle.EventBus.Transports.Azure.EventHubs.IotHub;
using SC = Korra.Processor.KorraProcessorSerializerContext;

namespace Korra.Processor;

[ConsumerName(EventHubConsumerClient.DefaultConsumerGroupName)]
internal class KorraIotEventsConsumer(KorraDashboardClient client, ILogger<KorraIotEventsConsumer> logger) : IEventConsumer<KorraIotHubEvent>
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
                    await HandleTwinChangeAsync(context, evt.Source, evt.Event!, cancellationToken);
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

        if (logger.IsEnabled(LogLevel.Debug))
        {
            logger.LogDebug("Received Telemetry from {DeviceId}\r\nEnqueued: {EnqueuedTime}\r\nTelemetry:{Telemetry}",
                            deviceId,
                            enqueued,
                            JsonSerializer.Serialize(incoming, SC.Default.KorraIotHubTelemetry));
        }

        var appKind = incoming.AppKind switch
        {
            KorraIotHubTelemetryAppKind.Keeper => KorraAppKind.Keeper,
            KorraIotHubTelemetryAppKind.Pot => KorraAppKind.Pot,
            null => incoming.PH is not null ? KorraAppKind.Pot : KorraAppKind.Keeper,
            _ => throw new NotImplementedException(),
        };
        var telemetryId = context.GetIotHubMessageId() ?? Tingle.Extensions.Primitives.Ksuid.Generate(incoming.Created);

        var telemetry = new KorraTelemetry
        {
            Id = telemetryId,
            DeviceId = deviceId,
            Received = enqueued?.ToUniversalTime(),
            AppKind = appKind,
            // we assume the units for this do not vary, otherwise we would need to convert
            Temperature = incoming.Temperature?.Value,
            Humidity = incoming.Humidity?.Value,
            Moisture = incoming.Moisture?.Value,
            PH = incoming.PH?.Value,
        };

        try
        {
            await client.SendTelemetryAsync(telemetry, cancellationToken);
        }
        catch (Exception ex)
        {
            logger.LogInformation(ex, "Forwarding telemetry failed. Shall safely proceed");
        }
    }

    internal virtual async Task HandleTwinChangeAsync(EventContext context,
                                                      IotHubEventMessageSource source,
                                                      IotHubOperationalEvent ope,
                                                      CancellationToken cancellationToken = default)
    {
        if (logger.IsEnabled(LogLevel.Debug))
        {
            logger.LogDebug("{Source} event received of type '{Type}' from '{DeviceId}'",
                            source,
                            ope.Type,
                            ope.DeviceId);
        }

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

        try
        {
            await client.SendOperationalEventAsync(@event, cancellationToken);
        }
        catch (Exception ex)
        {
            logger.LogInformation(ex, "Forwarding operational event failed. Shall safely proceed");
        }
    }
}
