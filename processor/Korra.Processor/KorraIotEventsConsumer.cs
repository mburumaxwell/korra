using System.Text.Json;
using Azure.Messaging.EventHubs.Consumer;
using Tingle.EventBus;
using Tingle.EventBus.Configuration;
using Tingle.EventBus.Transports.Azure.EventHubs;
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

        var typeRaw = context.GetEventData().GetPropertyValue<string>("type");
        var type = Enum.TryParse<KorraIotHubTelemetryType>(typeRaw, ignoreCase: true, out var value)
                 ? value : KorraIotHubTelemetryType.Sensors;

        var appKind = incoming.AppKind switch
        {
            KorraIotHubTelemetryAppKind.Keeper => KorraAppKind.Keeper,
            KorraIotHubTelemetryAppKind.Pot => KorraAppKind.Pot,
            null => incoming.PH is not null ? KorraAppKind.Pot : KorraAppKind.Keeper,
            _ => throw new NotImplementedException(),
        };
        // var telemetryId = Tingle.Extensions.Primitives.Ksuid.Generate(incoming.Created);
        var telemetryId = $"{context.GetEventData().SequenceNumber}";

        if (type is KorraIotHubTelemetryType.Sensors)
        {
            var sensors = new KorraTelemetrySensors
            {
                Id = telemetryId,
                DeviceId = deviceId,
                Created = new DateTimeOffset(incoming.Created, TimeSpan.Zero),
                Received = enqueued?.ToUniversalTime(),
                AppKind = appKind,
                // we assume the units for this do not vary, otherwise we would need to convert
                Temperature = incoming.Temperature?.Value,
                Humidity = incoming.Humidity?.Value,
                Moisture = incoming.Moisture?.Value,
                PH = incoming.PH?.Value,
            };

            logger.LogInformation("Forwarding sensors telemetry from {DeviceId} (dated: {Created:o})", deviceId, sensors.Created);
            if (logger.IsEnabled(LogLevel.Debug))
            {
                logger.LogDebug("{Telemetry}", JsonSerializer.Serialize(sensors, SC.Default.KorraTelemetrySensors));
            }
            await dashboardClient.SendAsync(sensors, cancellationToken);

            var tbp = new System.Text.Json.Nodes.JsonObject
            {
                ["id"] = sensors.Id,
                // tinybird expects a specific format for date-time
                ["timestamp"] = sensors.Created.ToString("yyyy-MM-dd'T'HH:mm:ss"),
                ["device_id"] = sensors.DeviceId,
                ["app_kind"] = sensors.AppKind.GetEnumMemberAttrValueOrDefault(),
                ["temperature"] = sensors.Temperature,
                ["humidity"] = sensors.Humidity,
                ["moisture"] = sensors.Moisture,
                ["ph"] = sensors.PH,
            };
            await tinybirdClient.SendAsync("telemetry", tbp, cancellationToken);
        }
        else if (type is KorraIotHubTelemetryType.Actuators)
        {
            var actuators = new KorraTelemetryActuators
            {
                Id = telemetryId,
                DeviceId = deviceId,
                Created = new DateTimeOffset(incoming.Created, TimeSpan.Zero),
                Received = enqueued?.ToUniversalTime(),
                AppKind = appKind,
                Pump = incoming.Pump,
                Fan = incoming.Fan,
            };

            logger.LogInformation("Forwarding actuator telemetry from {DeviceId} (dated: {Created:o})", deviceId, actuators.Created);
            if (logger.IsEnabled(LogLevel.Debug))
            {
                logger.LogDebug("{Telemetry}", JsonSerializer.Serialize(actuators, SC.Default.KorraTelemetryActuators));
            }
            await dashboardClient.SendAsync(actuators, cancellationToken);

            var tbp = new System.Text.Json.Nodes.JsonObject
            {
                ["id"] = actuators.Id,
                // tinybird expects a specific format for date-time
                ["timestamp"] = actuators.Created.ToString("yyyy-MM-dd'T'HH:mm:ss"),
                ["device_id"] = actuators.DeviceId,
                ["app_kind"] = actuators.AppKind.GetEnumMemberAttrValueOrDefault(),
                ["pump_duration"] = actuators.Pump?.Duration,
                ["pump_quantity"] = actuators.Pump?.Quantity,
                ["fan_duration"] = actuators.Fan?.Duration,
                ["fan_quantity"] = actuators.Fan?.Quantity,
            };
            await tinybirdClient.SendAsync("actuator_telemetry", tbp, cancellationToken);
        }
        else
        {
            throw new NotSupportedException($"Unsupported telemetry type: {type}");
        }
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

        var enqueued = context.GetIotHubEnqueuedTime();
        var @event = new KorraOperationalEvent
        {
            Type = type.Value,
            DeviceId = ope.DeviceId,
            SequenceNumber = ope.Payload.SequenceNumber,
            Received = enqueued?.ToUniversalTime(),
        };

        logger.LogInformation("Forwarding operational event for {DeviceId} (enqueued: {Enqueued:o})", ope.DeviceId, @event.Received);
        if (logger.IsEnabled(LogLevel.Debug))
        {
            logger.LogDebug("{Event}", JsonSerializer.Serialize(@event, SC.Default.KorraOperationalEvent));
        }
        await dashboardClient.SendAsync(@event, cancellationToken);
    }
}
