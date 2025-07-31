@description('Location for all resources.')
param location string = resourceGroup().location

@description('Name of the resources.')
@minLength(4)
@maxLength(15)
param name string = 'korra'

type BlobContainerDefinition = { name: string, public: bool?, nameDev: string? }

param blobContainers BlobContainerDefinition[] = [
  { name: 'iothub-checkpoints' }
  { name: 'iothub-checkpoints-dev' }
]

var administratorLoginPasswordMongo = '${skip(uniqueString(resourceGroup().id), 5)}^${uniqueString('mongo-password', resourceGroup().id)}' // e.g. abcde%zecnx476et7xm (19 characters)

/* Managed Identity */
resource managedIdentity 'Microsoft.ManagedIdentity/userAssignedIdentities@2023-01-31' = {
  name: name
  location: location
}

/* Key Vault */
resource keyVault 'Microsoft.KeyVault/vaults@2023-07-01' = {
  name: '${name}store'
  location: location
  properties: {
    tenantId: subscription().tenantId
    sku: { name: 'standard', family: 'A' }
    enabledForDeployment: true
    enabledForDiskEncryption: true
    enabledForTemplateDeployment: true
    enableRbacAuthorization: true
    enableSoftDelete: true
    softDeleteRetentionInDays: 90
  }

  resource mongoPasswordSecret 'secrets' = {
    name: 'mongo-password'
    properties: { contentType: 'text/plain', value: administratorLoginPasswordMongo }
  }
}

/* Storage Account */
resource storageAccount 'Microsoft.Storage/storageAccounts@2023-05-01' = {
  name: '${name}store' // korra is already taken
  location: location
  kind: 'StorageV2'
  properties: {
    accessTier: 'Hot'
    supportsHttpsTrafficOnly: true
    allowBlobPublicAccess: true // FrontDoor does not work without this
    networkAcls: { bypass: 'AzureServices', defaultAction: 'Allow' }
  }
  sku: { name: 'Standard_LRS' }

  resource blobServices 'blobServices' existing = {
    name: 'default'

    resource containers 'containers' = [
      for bc in blobContainers: {
        name: bc.name
        properties: {
          defaultEncryptionScope: '$account-encryption-key'
          denyEncryptionScopeOverride: false
          publicAccess: (bc.?public ?? false) ? 'Blob' : 'None'
        }
      }
    ]
  }
}

/* IoT Hub */
resource iotHub 'Microsoft.Devices/IotHubs@2023-06-30' = {
  name: name
  location: location
  properties: {
    features: 'GWV2, RootCertificateV2'
    eventHubEndpoints: { events: { retentionTimeInDays: 1, partitionCount: 2 } }
    routing: {
      routes: [
        {
          name: 'DeviceTwinEvents'
          source: 'TwinChangeEvents'
          condition: 'true'
          endpointNames: ['events']
          isEnabled: true
        }
        {
          name: 'DeviceConnectionStateEvents'
          source: 'DeviceConnectionStateEvents'
          condition: 'true'
          endpointNames: ['events']
          isEnabled: true
        }
      ]
      fallbackRoute: {
        name: '$fallback'
        source: 'DeviceMessages'
        condition: 'true'
        endpointNames: ['events']
        isEnabled: true
      }
    }
    storageEndpoints: { '$default': { sasTtlAsIso8601: 'PT1H', connectionString: '', containerName: '' } }
    messagingEndpoints: {
      fileNotifications: { lockDurationAsIso8601: 'PT1M', ttlAsIso8601: 'PT1H', maxDeliveryCount: 10 }
    }
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
    iotHubs: [
      {
        location: location
        #disable-next-line BCP037
        authenticationType: 'KeyBased'
        connectionString: 'HostName=${iotHub.properties.hostName};SharedAccessKeyName=iothubowner;SharedAccessKey=${iotHub.listkeys().value[0].primaryKey}'
      }
    ]
    allocationPolicy: 'Hashed'
  }
  sku: { name: 'S1', capacity: 1 }
}

/* IoT Device Update */
resource iotUpdateAccount 'Microsoft.DeviceUpdate/accounts@2023-07-01' = {
  name: name
  location: location
  properties: {
    sku: 'Free'
  }

  /* Cannot create the instance with a free tier iot hub. Till then we do the magic manually */
  // resource instance 'instances' = {
  //   name: name
  //   location: location
  //   properties: {
  //     enableDiagnostics: false
  //     iotHubs: [{ resourceId: iotHub.id }]
  //   }
  // }

  identity: { type: 'UserAssigned', userAssignedIdentities: { '${managedIdentity.id}': {} } }
}

/* LogAnalytics */
resource logAnalyticsWorkspace 'Microsoft.OperationalInsights/workspaces@2022-10-01' = {
  name: name
  location: location
  properties: {
    sku: { name: 'PerGB2018' }
    workspaceCapping: {
      dailyQuotaGb: json('0.167') // low so as not to pass the 5GB limit per subscription
    }
  }
}

/* AppService Plan */
resource appServicePlan 'Microsoft.Web/serverfarms@2024-04-01' = {
  name: name
  location: location
  kind: 'linux'
  properties: {
    reserved: true
    zoneRedundant: false // only for Premium tiers
  }
  sku: {
    name: 'F1'
  }
}

/* Container App Environment */
resource appEnvironment 'Microsoft.App/managedEnvironments@2023-05-01' = {
  name: name
  location: location
  properties: {
    appLogsConfiguration: {
      destination: 'log-analytics'
      logAnalyticsConfiguration: {
        customerId: logAnalyticsWorkspace.properties.customerId
        sharedKey: logAnalyticsWorkspace.listKeys().primarySharedKey
      }
    }
  }
}

/* MongoDB Cluster */
resource mongoCluster 'Microsoft.DocumentDB/mongoClusters@2024-07-01' = {
  name: name
  location: 'francecentral' // no free tier in UK yet
  properties: {
    #disable-next-line use-secure-value-for-secure-inputs
    administrator: { userName: 'puppy', password: administratorLoginPasswordMongo }
    serverVersion: '8.0'
    compute: { tier: 'Free' }
    storage: { sizeGb: 32 }
    sharding: { shardCount: 1 }
    highAvailability: { targetMode: 'Disabled' }
  }

  // resource allowAzure 'firewallRules' = {
  //   name: 'AllowAllAzureServicesAndResourcesWithinAzureIps'
  //   properties: { endIpAddress: '0.0.0.0', startIpAddress: '0.0.0.0' }
  // }

  // allowing all IPs for now, because we deploy the webapp on Vercel and it needs to access the database
  // this is the least complex solution, but not the most secure
  resource allowAll 'firewallRules' = {
    name: 'AllowAll_IPs'
    properties: { startIpAddress: '0.0.0.0', endIpAddress: '255.255.255.255' }
  }
}

/* Application Insights */
resource appInsightsWebsite 'Microsoft.Insights/components@2020-02-02' = {
  name: name
  location: location
  kind: 'web'
  properties: { Application_Type: 'web', WorkspaceResourceId: logAnalyticsWorkspace.id }
}

/** WebApps */
resource webApp 'Microsoft.Web/sites@2024-04-01' = {
  name: name
  location: location
  properties: {
    serverFarmId: appServicePlan.id
    clientAffinityEnabled: false
    httpsOnly: true
    publicNetworkAccess: 'Enabled'
    autoGeneratedDomainNameLabelScope: 'ResourceGroupReuse'
    siteConfig: { linuxFxVersion: 'NODE|22-lts' }
  }

  resource scmCredentials 'basicPublishingCredentialsPolicies' = { name: 'scm', properties: { allow: false } }
  resource ftpCredentials 'basicPublishingCredentialsPolicies' = { name: 'ftp', properties: { allow: false } }

  resource siteConfig 'config' = {
    name: 'web'
    properties: { linuxFxVersion: 'NODE|22-lts', appCommandLine: 'node server.js' }
  }
}

// custom domain is not mapped here because DNS for maxwellweru.com is controlled in another repo

/* Role Assignments */
var roles = [
  { name: 'IoT Hub Data Contributor', id: '4fc6c259-987e-4a07-842e-c321cc9d413f' } // Allows for full access to IoT Hub data plane operations.
  { name: 'Device Provisioning Service Data Contributor', id: 'dfce44e4-17b7-4bd1-a6d1-04996ec95633' } // Allows for full access to Device Provisioning Service data-plane operations.
  { name: 'Device Update Administrator', id: '02ca0879-e8e4-47a5-a61e-5c618b76e64a' } // Gives you full access to management and content operations
  { name: 'Storage Blob Data Contributor', id: 'ba92f5b4-2d11-453d-a403-e96b0029c9fe' } // Read, write, and delete Azure Storage containers and blobs.
]

resource roleAssignments 'Microsoft.Authorization/roleAssignments@2022-04-01' = [
  for role in roles: {
    name: guid(managedIdentity.id, role.name)
    scope: resourceGroup()
    properties: {
      roleDefinitionId: subscriptionResourceId('Microsoft.Authorization/roleDefinitions', role.id)
      principalId: managedIdentity.properties.principalId
    }
  }
]

output managedIdentityPrincipalId string = managedIdentity.properties.principalId
output iotHubHostName string = iotHub.properties.hostName
output iotDpsServiceHostName string = iotDps.properties.serviceOperationsHostName
output mongoConnectionString string = replace(
  mongoCluster.properties.connectionString,
  '<user>',
  mongoCluster.properties.administrator.userName
)
output webAppHostName string = webApp.properties.defaultHostName
