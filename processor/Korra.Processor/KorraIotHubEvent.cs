using System.Runtime.Serialization;
using System.Text.Json.Serialization;
using Tingle.EventBus.Transports.Azure.EventHubs.IotHub;
using Tingle.Extensions.Primitives.Converters;

namespace Korra.Processor;

public record KorraIotHubEvent : IotHubEvent { }

/// <param name="Timestamp"></param>
/// <param name="Created">When the telemetry was created (always UTC)</param>
/// <param name="Temperature">Measured in °C</param>
/// <param name="Humidity">Relative humidity (%)</param>
/// <param name="Moisture">Percentage (%) of water in the soil</param>
/// <param name="PH"></param>
public record KorraIotHubTelemetry(
    [property: JsonPropertyName("timestamp")] ulong Timestamp,
    [property: JsonPropertyName("created")] DateTime Created,
    [property: JsonPropertyName("app_kind")] KorraIotHubTelemetryAppKind? AppKind,
    [property: JsonPropertyName("temperature")] KorraIotHubTelemetrySensorValue? Temperature,
    [property: JsonPropertyName("humidity")] KorraIotHubTelemetrySensorValue? Humidity,
    [property: JsonPropertyName("moisture")] KorraIotHubTelemetrySensorValue? Moisture,
    [property: JsonPropertyName("ph")] KorraIotHubTelemetrySensorValue? PH,
    [property: JsonPropertyName("pump")] KorraIotHubTelemetryActuatorValue? Pump,
    [property: JsonPropertyName("fan")] KorraIotHubTelemetryActuatorValue? Fan);

[JsonConverter(typeof(JsonStringEnumMemberConverter<KorraIotHubTelemetryAppKind>))]
public enum KorraIotHubTelemetryAppKind
{
    [EnumMember(Value = "keeper")] Keeper,
    [EnumMember(Value = "pot")] Pot,
}

[JsonConverter(typeof(JsonStringEnumMemberConverter<KorraIotHubTelemetryType>))]
public enum KorraIotHubTelemetryType
{
    [EnumMember(Value = "sensors")] Sensors,
    [EnumMember(Value = "actuators")] Actuators,
}

public record KorraIotHubTelemetryActuatorValue(
    [property: JsonPropertyName("duration")] int Duration,
    [property: JsonPropertyName("quantity")] float? Quantity);

public record KorraIotHubTelemetrySensorValue(
    [property: JsonPropertyName("value")] float Value,
    [property: JsonPropertyName("unit")] string? Unit);
