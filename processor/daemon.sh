#!/usr/bin/env bash

# This command runs the Korra processor in a Docker container with the necessary environment variables set.
# Remember to replace the environment variables with your actual values before running the command.
# You can add "--rm -it" to the command to remove the container after it stops and to run it interactively, respectively.

# Example values:
# - BlobStorage__ConnectionString: "DefaultEndpointsProtocol=https;EndpointSuffix=core.windows.net;AccountName=abcd;AccountKey=AAAAAAAAAAAAAAAAAAAAAA=="
# - IotHub__EventHubs__ConnectionString: "Endpoint=sb://abcd.servicebus.windows.net/;SharedAccessKeyName=xyz;SharedAccessKey=AAAAAAAAAAAAAAAAAAAAAA=="
# - IotHub__EventHubs__Checkpoints__BlobContainerName: "iothub-checkpoints"
# - IotHub__EventHubs__HubName: "iothub-ehub-test-dev-0000000-0aaaa000aa"
# - Dashboard__Endpoint: "https://korra.maxwellweru.com"
# - Dashboard__ApiKey: "123-strong"

sudo docker run \
-e BlobStorage__ConnectionString="<your-blob-storage-connection-string>" \
-e IotHub__EventHubs__ConnectionString="<your-event-hubs-connection-string>" \
-e IotHub__EventHubs__Checkpoints__BlobContainerName="<your-blob-container-name>" \
-e IotHub__EventHubs__HubName="<event-hub-name>" \
-e Dashboard__Endpoint="<your-base-url-without-trailing-slash>" \
-e Dashboard__ApiKey="<your-api-key>" \
-e Tinybird__Token="<get-from-tinybird-dashboard>" \
ghcr.io/mburumaxwell/korra/processor:latest
