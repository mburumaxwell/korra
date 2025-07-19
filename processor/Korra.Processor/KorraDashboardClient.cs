using System.Runtime.Serialization;
using System.Text.Json.Serialization;
using Microsoft.Extensions.Options;
using Tingle.Extensions.Http;
using Tingle.Extensions.Primitives.Converters;
using SC = Korra.Processor.KorraProcessorSerializerContext;

namespace Korra.Processor;

public class AbstractKorraTelemetry
{
    [JsonPropertyName("id")]
    public required string Id { get; set; }

    [JsonPropertyName("device_id")]
    public required string DeviceId { get; set; }

    [JsonPropertyName("created")]
    public required DateTimeOffset Created { get; set; }

    [JsonPropertyName("received")]
    public required DateTimeOffset? Received { get; set; }

    [JsonPropertyName("app_kind")]
    public required KorraAppKind AppKind { get; set; }
}

public class KorraTelemetrySensors : AbstractKorraTelemetry
{

    [JsonPropertyName("temperature")]
    public required float? Temperature { get; set; }

    [JsonPropertyName("humidity")]
    public required float? Humidity { get; set; }

    [JsonPropertyName("moisture")]
    public required float? Moisture { get; set; }

    [JsonPropertyName("ph")]
    public required float? PH { get; set; }
}

public class KorraTelemetryActuators : AbstractKorraTelemetry
{
    [JsonPropertyName("pump_duration")]
    public required int? PumpDuration { get; set; }

    [JsonPropertyName("pump_quantity")]
    public required float? PumpQuantity { get; set; }

    [JsonPropertyName("fan_duration")]
    public required int? FanDuration { get; set; }

    [JsonPropertyName("fan_quantity")]
    public required float? FanQuantity { get; set; }
}

[JsonConverter(typeof(JsonStringEnumMemberConverter<KorraAppKind>))]
public enum KorraAppKind
{
    [EnumMember(Value = "keeper")] Keeper,
    [EnumMember(Value = "pot")] Pot,
}

public class KorraOperationalEvent
{
    [JsonPropertyName("type")]
    public required KorraOperationalEventType Type { get; set; }

    [JsonPropertyName("device_id")]
    public required string DeviceId { get; set; }

    [JsonPropertyName("sequence_number")]
    public required string? SequenceNumber { get; set; }

    [JsonPropertyName("received")]
    public required DateTimeOffset? Received { get; set; }
}

[JsonConverter(typeof(JsonStringEnumMemberConverter<KorraOperationalEventType>))]
public enum KorraOperationalEventType
{
    [EnumMember(Value = "connected")] Connected,
    [EnumMember(Value = "disconnected")] Disconnected,
    [EnumMember(Value = "twin.updated")] TwinUpdated,
}

public class KorraDashboardClient(HttpClient httpClient, IOptionsSnapshot<KorraDashboardClientOptions> optionsAccessor)
    : AbstractHttpApiClient<KorraDashboardClientOptions>(httpClient, optionsAccessor)
{
    public async Task SendAsync(KorraTelemetrySensors telemetry, CancellationToken cancellationToken = default)
    {
        var content = MakeJsonContent(telemetry, SC.Default.KorraTelemetrySensors);
        var request = new HttpRequestMessage(HttpMethod.Post, "/api/processor/telemetry/sensors") { Content = content, };
        var response = await SendAsync(request, SC.Default.KorraDashboardResponse, cancellationToken);
        response.EnsureSuccess();
    }

    public async Task SendAsync(KorraTelemetryActuators telemetry, CancellationToken cancellationToken = default)
    {
        var content = MakeJsonContent(telemetry, SC.Default.KorraTelemetryActuators);
        var request = new HttpRequestMessage(HttpMethod.Post, "/api/processor/telemetry/actuators") { Content = content, };
        var response = await SendAsync(request, SC.Default.KorraDashboardResponse, cancellationToken);
        response.EnsureSuccess();
    }

    public async Task SendAsync(KorraOperationalEvent @event, CancellationToken cancellationToken = default)
    {
        var content = MakeJsonContent(@event, SC.Default.KorraOperationalEvent);
        var request = new HttpRequestMessage(HttpMethod.Post, "/api/processor/operational-event") { Content = content, };
        var response = await SendAsync(request, SC.Default.KorraDashboardResponse, cancellationToken);
        response.EnsureSuccess();
    }
}

public class KorraDashboardResponse { }

public class KorraDashboardClientOptions : AbstractHttpApiClientOptions
{
}
