using System.Text.Json;
using System.Text.Json.Serialization;

namespace Korra.Processor;

[JsonSerializable(typeof(KorraTelemetrySensors))]
[JsonSerializable(typeof(KorraTelemetryActuators))]
[JsonSerializable(typeof(KorraIotHubTelemetry))]
[JsonSerializable(typeof(KorraOperationalEvent))]
[JsonSerializable(typeof(KorraDashboardResponse))]
[JsonSerializable(typeof(System.Text.Json.Nodes.JsonObject))]

[JsonSourceGenerationOptions(
    AllowTrailingCommas = true,
    ReadCommentHandling = JsonCommentHandling.Skip,

    // Ignore default values to reduce the data sent after serialization
    DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,

    // Do not indent content to reduce data usage
    WriteIndented = false,

    // Use SnakeCase because it is what the server provides
    PropertyNamingPolicy = JsonKnownNamingPolicy.SnakeCaseLower,
    DictionaryKeyPolicy = JsonKnownNamingPolicy.Unspecified
)]
internal partial class KorraProcessorSerializerContext : JsonSerializerContext { }
