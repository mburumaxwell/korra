@description('Location for all resources.')
param location string = resourceGroup().location

@description('Name of the resources.')
@minLength(4)
@maxLength(23)
param name string = 'korra'

/* Managed Identity */
resource managedIdentity 'Microsoft.ManagedIdentity/userAssignedIdentities@2023-01-31' = { name: name, location: location }

/* IoT Hub */
resource iotHub 'Microsoft.Devices/IotHubs@2023-06-30' = {
  name: name
  location: location
  properties: {
    features: 'GWV2, RootCertificateV2'
    eventHubEndpoints: { events: { retentionTimeInDays: 1, partitionCount: 2 } }
    routing: {
      enrichments: [{ key: 'deviceId', value: '$twin.tags.deviceId', endpointNames: ['events'] }]
      routes: [
        { name: 'DeviceLifecycleEvents', source: 'DeviceLifecycleEvents', condition: 'true', endpointNames: ['events'], isEnabled: true }
        { name: 'DeviceTwinEvents', source: 'TwinChangeEvents', condition: 'true', endpointNames: ['events'], isEnabled: true }
        { name: 'DeviceConnectionStateEvents', source: 'DeviceConnectionStateEvents', condition: 'true', endpointNames: ['events'], isEnabled: true }
      ]
      fallbackRoute: { name: '$fallback', source: 'DeviceMessages', condition: 'true', endpointNames: ['events'], isEnabled: true }
    }
    storageEndpoints: { '$default': { sasTtlAsIso8601: 'PT1H', connectionString: '', containerName: '' } }
    messagingEndpoints: { fileNotifications: { lockDurationAsIso8601: 'PT1M', ttlAsIso8601: 'PT1H', maxDeliveryCount: 10 } }
    enableFileUploadNotifications: false
    cloudToDevice: {
      maxDeliveryCount: 10
      defaultTtlAsIso8601: 'PT1H'
      feedback: { lockDurationAsIso8601: 'PT1M', ttlAsIso8601: 'PT1H', maxDeliveryCount: 10 }
    }
  }
  sku: { name: 'F1', capacity: 1 }
  identity: { type: 'UserAssigned', userAssignedIdentities: { '${managedIdentity.id}': {} } }
}

/* IoT Device Provisioning (DPS) */
resource iotDps 'Microsoft.Devices/provisioningServices@2022-12-12' = {
  name: name
  location: location
  properties: {
    iotHubs: [{
      location: location
      #disable-next-line BCP037
      authenticationType: 'KeyBased'
      connectionString: 'HostName=${iotHub.properties.hostName};SharedAccessKeyName=iothubowner;SharedAccessKey=${iotHub.listkeys().value[0].primaryKey}'
    }]
    allocationPolicy: 'Hashed'
  }
  sku: { name: 'S1', capacity: 1 }
}

/* Static WebApp */
resource staticWebApp 'Microsoft.Web/staticSites@2024-11-01' = {
  name: name
  location: 'westeurope' // not available in UK yet
  properties: {
    repositoryUrl: 'https://github.com/mburumaxwell/korra'
    branch: 'main'
    stagingEnvironmentPolicy: 'Enabled'
    allowConfigFileUpdates: true
    provider: 'GitHub'
    enterpriseGradeCdnStatus: 'Disabled'
    #disable-next-line BCP037
    deploymentAuthPolicy: 'DeploymentToken'
    #disable-next-line BCP037
    #disable-next-line BCP037
    trafficSplitting: { environmentDistribution: { default: 100 } }
    publicNetworkAccess: 'Enabled'
  }
  sku: { name: 'Free', tier: 'Free' }
  // identity: { type: 'UserAssigned', userAssignedIdentities: { '${managedIdentity.id}': {} } }
}

// custom domain is not mapped here because DNS for maxwellweru.com is controlled in another repo

/* Role Assignments */
var roles = [
  { name: 'IoT Hub Data Contributor', id: '4fc6c259-987e-4a07-842e-c321cc9d413f' } // Allows for full access to IoT Hub data plane operations.
  { name: 'Device Provisioning Service Data Contributor', id: 'dfce44e4-17b7-4bd1-a6d1-04996ec95633' } // Allows for full access to Device Provisioning Service data-plane operations.
]

resource roleAssignments 'Microsoft.Authorization/roleAssignments@2022-04-01' = [for role in roles: {
  name: guid(managedIdentity.id, role.name)
  scope: resourceGroup()
  properties: {
    roleDefinitionId: subscriptionResourceId('Microsoft.Authorization/roleDefinitions', role.id)
    principalId: managedIdentity.properties.principalId
  }
}]

output managedIdentityPrincipalId string = managedIdentity.properties.principalId
output iotHubHostName string = iotHub.properties.hostName
output iotDpsServiceHostName string = iotDps.properties.serviceOperationsHostName
output staticWebAppHostName string = staticWebApp.properties.defaultHostname
