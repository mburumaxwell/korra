@description('Location for all resources.')
param location string = resourceGroup().location

@description('The suffix used to name the app as a reviewApp')
param reviewAppNameSuffix string = ''

@description('The tag of the container. E.g. 1.2.0')
param containerImageTag string = '#{DOCKER_IMAGE_TAG}#'

var isReviewApp = reviewAppNameSuffix != null && !empty(reviewAppNameSuffix)
var replicaTimeout = 60 * 10 // 10 minutes, max

// Existing resources
resource managedIdentity 'Microsoft.ManagedIdentity/userAssignedIdentities@2023-01-31' existing = {
  name: 'korra'
}
resource storageAccount 'Microsoft.Storage/storageAccounts@2023-01-01' existing = { name: 'korrastore' }
resource iotHub 'Microsoft.Devices/IotHubs@2023-06-30' existing = { name: 'korra' }
resource appEnvironment 'Microsoft.App/managedEnvironments@2023-05-01' existing = { name: 'korra' }

resource processor 'Microsoft.App/jobs@2025-01-01' = {
  name: 'processor${reviewAppNameSuffix}'
  location: location
  properties: {
    environmentId: appEnvironment.id
    configuration: {
      triggerType: 'Schedule'
      scheduleTriggerConfig: {
        cronExpression: '0 */2 * * *' // every 2 hours
        parallelism: 1
        replicaCompletionCount: 1
      }
      manualTriggerConfig: { parallelism: 1, replicaCompletionCount: 1 }
      replicaTimeout: replicaTimeout
      replicaRetryLimit: 1
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

        #disable-next-line use-secure-value-for-secure-inputs
        { name: 'tinybird-token', value: '#{TINYBIRD_TOKEN}#' }
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
            { name: 'IotHub__EventHubs__Checkpoints__HubName', value: iotHub.properties.eventHubEndpoints.events.path }

            { name: 'Dashboard__Endpoint', secretRef: 'dashboard-endpoint' }
            { name: 'Dashboard__ApiKey', secretRef: 'dashboard-api-key' }

            { name: 'Tinybird__Token', secretRef: 'tinybird-token' }

            // run for 5 seconds less than the replica timeout, to complete gracefully
            { name: 'JOB_DURATION_SECONDS', value: string(replicaTimeout - 5) }
          ]
          resources: { cpu: json('0.25'), memory: '0.5Gi' }
        }
      ]
    }
  }
  identity: { type: 'UserAssigned', userAssignedIdentities: { '${managedIdentity.id}': {} } }
}
