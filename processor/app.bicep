@description('Location for all resources.')
param location string = resourceGroup().location

@description('The suffix used to name the app as a reviewApp')
param reviewAppNameSuffix string = ''

@description('The tag of the container. E.g. 1.2.0')
param containerImageTag string = '#{DOCKER_IMAGE_TAG}#'

var isReviewApp = reviewAppNameSuffix != null && !empty(reviewAppNameSuffix)

// Existing resources
resource managedIdentity 'Microsoft.ManagedIdentity/userAssignedIdentities@2023-01-31' existing = {
  name: 'korra'
}
resource storageAccount 'Microsoft.Storage/storageAccounts@2023-01-01' existing = { name: 'korrastore' }
resource iotHub 'Microsoft.Devices/IotHubs@2023-06-30' existing = { name: 'korra' }
resource appEnvironment 'Microsoft.App/managedEnvironments@2023-05-01' existing = { name: 'korra' }

resource processor 'Microsoft.App/containerApps@2025-01-01' = {
  name: 'processor${reviewAppNameSuffix}'
  location: location
  properties: {
    environmentId: appEnvironment.id
    configuration: {
      secrets: [
        {
          name: 'connection-strings-iot-hub-event-hub'
          value: join(
            [
              'Endpoint=${iotHub.properties.eventHubEndpoints.events.endpoint}'
              'SharedAccessKeyName=iothubowner'
              'SharedAccessKey=${iotHub.listkeys().value[0].primaryKey}'
              // cannot set path here and in the ENV so we remove this since the ENV one is required
              // 'EntityPath=${iotHub.properties.eventHubEndpoints.events.path}'
            ],
            ';'
          )
        }

        #disable-next-line use-secure-value-for-secure-inputs
        { name: 'dashboard-endpoint', value: 'https://korra.maxwellweru.com' } // easier to override as a secret in the Azure portal
        #disable-next-line use-secure-value-for-secure-inputs
        { name: 'dashboard-api-key', value: '#{PROCESSOR_API_KEY}#' }
      ]
    }
    template: {
      containers: [
        {
          image: 'ghcr.io/mburumaxwell/korra/processor:${containerImageTag}'
          name: 'processor'
          env: [
            // Specify the User-Assigned Managed Identity to use. Without this, the app attempt to use the system assigned one.
            { name: 'AZURE_CLIENT_ID', value: managedIdentity.properties.clientId }

            { name: 'BlobStorage__Endpoint', value: storageAccount.properties.primaryEndpoints.blob }
            { name: 'IotHub__EventHubs__ConnectionString', secretRef: 'connection-strings-iot-hub-event-hub' }
            {
              name: 'IotHub__EventHubs__Checkpoints__BlobContainerName'
              value: isReviewApp ? 'iothub-checkpoints-review' : 'iothub-checkpoints'
            }
            { name: 'IotHub__EventHubs__HubName', value: iotHub.properties.eventHubEndpoints.events.path }

            { name: 'Dashboard__Endpoint', secretRef: 'dashboard-endpoint' }
            { name: 'Dashboard__ApiKey', secretRef: 'dashboard-api-key' }
          ]
          resources: { cpu: json('0.25'), memory: '0.5Gi' }
        }
      ]
      scale: {
        minReplicas: 0
        maxReplicas: 1
        pollingInterval: 30 // 30 seconds, default is 30 seconds
        cooldownPeriod: 60 // 1 minute, default is 5 minutes
        rules: [
          {
            name: 'schedule'
            custom: {
              type: 'cron'
              metadata: {
                timezone: 'Europe/London'
                // Run every 15 minutes for 1 minute
                start: '0,15,30,45 * * * *'
                end: '1,16,31,46 * * * *'
                desiredReplicas: '1'
              }
            }
          }
        ]
      }
    }
  }
  identity: { type: 'UserAssigned', userAssignedIdentities: { '${managedIdentity.id}': {} } }
}
