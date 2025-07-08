using Microsoft.Extensions.Options;
using System.Text.Json.Nodes;
using Tingle.Extensions.Http;
using SC = Korra.Processor.KorraProcessorSerializerContext;

namespace Korra.Processor;

/// <summary>A client for interacting with Tinybird.</summary>
/// <param name="httpClient">The <see cref="HttpClient"/> for making requests.</param>
/// <param name="optionsAccessor">The accessor for the configuration options.</param>
public class TinybirdClient(HttpClient httpClient, IOptionsSnapshot<TinybirdClientOptions> optionsAccessor)
    : AbstractHttpApiClient<TinybirdClientOptions>(httpClient, optionsAccessor)
{
    /// <summary>Send data to a Data Source via the Events API.</summary>
    /// <param name="name">Name or ID of the target Data Source to append data to.</param>
    /// <param name="payload"></param>
    /// <param name="cancellationToken">The token to cancel the request.</param>
    public Task SendAsync(string name, JsonObject payload, CancellationToken cancellationToken = default)
        => SendAsync(name, MakeJsonContent(payload, SC.Default.JsonObject), cancellationToken);

    /// <summary>Send data to a Data Source via the Events API.</summary>
    /// <param name="name">Name or ID of the target Data Source to append data to.</param>
    /// <param name="content"></param>
    /// <param name="cancellationToken">The token to cancel the request.</param>
    internal async Task SendAsync(string name, HttpContent content, CancellationToken cancellationToken = default)
    {
        // `false` by default. Set to `true` to wait until the write is acknowledged by the database.
        // Enabling this flag makes possible to retry on database errors, but it introduces additional latency.
        // It's recommended to enable it in use cases in which data loss avoidance is critical.
        // It's recommended to disable it otherwise.
        bool wait = true;
        var @params = new Dictionary<string, string> { ["name"] = name, ["wait"] = wait.ToString().ToLowerInvariant() };
        var uri = QueryHelper.AddQueryString("/v0/events", @params);

        var request = new HttpRequestMessage(HttpMethod.Post, uri) { Content = content, };
        request.Headers.Authorization = new("Bearer", Options.Token);

        var response = await SendAsync(request, SC.Default.JsonObject, cancellationToken);
        response.EnsureSuccess();
    }
}

/// <summary>Configuration options for <see cref="TinybirdClient"/>.</summary>
public class TinybirdClientOptions : AbstractHttpApiClientOptions
{
    /// <summary>The Tinybird API token to use for authentication.</summary>
    public string? Token { get; set; }
}

internal class TinybirdClientConfigureOptions : IValidateOptions<TinybirdClientOptions>
{
    public ValidateOptionsResult Validate(string? name, TinybirdClientOptions options)
    {
        if (string.IsNullOrWhiteSpace(options.Token))
        {
            return ValidateOptionsResult.Fail($"{nameof(options.Token)} must be provided");
        }

        return ValidateOptionsResult.Success;
    }
}
