#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_cloud_provisioning, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/settings/settings.h>

#include <korra_credentials.h>
#include <korra_net_utils.h>

#include "korra_cloud_provisioning.h"

#define SETTINGS_NAME_HOSTNAME "azure-hub"
#define SETTINGS_NAME_DEVICEID "azure-deviceid"
#define DPS_HOSTNAME "global.azure-devices-provisioning.net"

#define DPS_CONNECT_TRIES 10
#define DPS_SLEEP_DURATION K_MSEC(500)

// official/ref docs
// - https://learn.microsoft.com/en-us/azure/iot/iot-mqtt-connect-to-iot-dps

// Username format -> {idScope}/registrations/{registration_id}/api-version=2019-03-31
#define USERNAME_FORMAT "%s/registrations/%s/api-version=2019-03-31"

/** The provisioning status type. */
enum provisioning_registration_status {
  PROVISIONING_REGISTRATION_STATUS_UNKNOWN = 0,

  /** Device has not yet come online. */
  PROVISIONING_REGISTRATION_STATUS_UNASSIGNED = 1,

  /** Device has connected to the DRS but IoT hub Id has not yet been returned to the device. */
  PROVISIONING_REGISTRATION_STATUS_ASSIGNING = 2,

  /** DRS successfully returned a device Id and connection string to the device. */
  PROVISIONING_REGISTRATION_STATUS_ASSIGNED = 3,

  /** Device enrollment failed. */
  PROVISIONING_REGISTRATION_STATUS_FAILED = 4,

  /** Device is disabled. */
  PROVISIONING_REGISTRATION_STATUS_DISABLED = 5,
};

/** The provisioning substatus type. */
enum provisioning_registration_sub_status {
  PROVISIONING_REGISTRATION_SUB_STATUS_UNKNOWN = 1,

  /** Device has been assigned to an IoT hub for the first time. */
  PROVISIONING_REGISTRATION_SUB_STATUS_INITIAL_ASSIGNMENT = 1,

  /**
   * Device has been assigned to a different IoT hub and its device data was migrated from the previously assigned IoT
   * hub. Device data was removed from the previously assigned IoT hub.
   */
  PROVISIONING_REGISTRATION_SUB_STATUS_DEVICE_DATA_MIGRATED = 2,

  /**
   * Device has been assigned to a different IoT hub and its device data was populated from the initial state stored
   * in the enrollment. Device data was removed from the previously assigned IoT hub.
   */
  PROVISIONING_REGISTRATION_SUB_STATUS_DEVICE_DATA_RESET = 3,
};

/** The result of a registration operation. */
struct device_provisioning_result {
  /** The registration id. */
  char *registration_id;

  /** The time when the device originally registered with the service. */
  time_t created_date_time_utc;

  /** The assigned Azure IoT hub. */
  char *assigned_hub;

  /** The Device Id. */
  char *device_id;

  /** The status of the operation. */
  enum provisioning_registration_status status;

  /** The substatus of the operation. */
  enum provisioning_registration_sub_status sub_status;

  /** The generation Id. */
  char *generation_id;

  /** The time when the device last refreshed the registration. */
  time_t last_updated_date_time_utc;

  /** Error code. */
  int error_code;

  /** Error message. */
  char *error_message;

  /** The Etag. */
  char *etag;

  /** The custom data returned from the webhook to the device. */
  char *json_payload;
};

/** Registration operation status. */
struct registration_operation_status {
  /** The operation Id. */
  char *operation_id;

  /** The device enrollment status. */
  enum provisioning_registration_status status;

  /** The device registration status. */
  struct device_provisioning_result registration_state;

  /** the Retry-After header. */
  char *retry_after;
};

static const sec_tag_t m_sec_tags[] = {
    KORRA_CREDENTIAL_AZURE_CA_TAG,
    KORRA_CREDENTIAL_DEVICE_TAG,
};

static uint8_t rx_buffer[256];
static uint8_t tx_buffer[256];
static struct mqtt_client client;
static struct sockaddr_storage broker = {0};
static struct pollfd sockfd[1] = {0};

static char *clientid, *username;
static size_t clientid_len, username_len;
static struct mqtt_utf8 mqtt_username;
static bool registration_requested = false;
static bool connected = false;

static int provisioning_info_save(const struct korra_cloud_provisioning_info *info);
static void provisioning_info_print(const struct korra_cloud_provisioning_info *info);
static int client_init(struct mqtt_client *client);
static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);

static void mqtt_service_handler(struct net_socket_service_event *pev);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_mqtt_dps, mqtt_service_handler, ARRAY_SIZE(sockfd));

int korra_cloud_provisioning_init(const char *regid, const size_t regid_len,
                                  struct korra_cloud_provisioning_info *result) {
  LOG_DBG("Initializing");

  // load hub hostname
  ssize_t ret_or_len = settings_load_one(SETTINGS_NAME_HOSTNAME, result->hostname, sizeof(result->hostname));
  if (ret_or_len < 0) {
    LOG_ERR("Unable to load " SETTINGS_NAME_HOSTNAME " from settings: %d", ret_or_len);
    return ret_or_len;
  }
  result->hostname_len = ret_or_len;

  // load device id
  ret_or_len = settings_load_one(SETTINGS_NAME_DEVICEID, result->id, sizeof(result->id));
  if (ret_or_len) {
    LOG_ERR("Unable to load " SETTINGS_NAME_DEVICEID " from settings: %d", ret_or_len);
    return ret_or_len;
  }
  result->id_len = ret_or_len;

  // determine if the provisioning info is valid and print
  result->valid = result->hostname_len > 0 && result->id_len > 0;
  provisioning_info_print(result);
  if (result->valid) return 0;

  // prepare client id as per spec
  clientid = (char *)regid;
  clientid_len = regid_len;

  // prepare username as per spec
  username_len = snprintf(NULL, 0, USERNAME_FORMAT, CONFIG_AZURE_IOT_DPS_ID_SCOPE, regid) + 1; // plus NULL
  username = k_malloc(username_len);
  if (username == NULL) {
    LOG_ERR("Unable to allocate %d bytes for username", username_len);
    return -ENOMEM;
  }
  username_len = snprintf(username, username_len, USERNAME_FORMAT, CONFIG_AZURE_IOT_DPS_ID_SCOPE, regid);

  mqtt_username.utf8 = username;
  mqtt_username.size = username_len;

  registration_requested = false;

  return 0;
}

int korra_cloud_provisioning_run(struct korra_cloud_provisioning_info *result) {
  if (result->valid) return 0;

  int ret, i = 0;

  while (i++ < DPS_CONNECT_TRIES && !connected) {

    ret = client_init(&client);
    if (ret != 0) {
      LOG_ERR("client_init failed: %d", ret);
      k_sleep(DPS_SLEEP_DURATION);
      continue;
    }

    ret = mqtt_connect(&client);
    if (ret != 0) {
      LOG_ERR("mqtt_connect failed: %d", ret);
      k_sleep(DPS_SLEEP_DURATION);
      continue;
    }

    sockfd[0].fd =
        client.transport.type == MQTT_TRANSPORT_SECURE ? client.transport.tls.sock : client.transport.tcp.sock;
    sockfd[0].events = POLLIN;

    ret = net_socket_service_register(&service_mqtt_dps, sockfd, ARRAY_SIZE(sockfd), /* user_data */ NULL);
    if (ret < 0) {
      LOG_ERR("Failed to register socket service: %d", ret);
      mqtt_abort(&client);
      k_sleep(DPS_SLEEP_DURATION);
      continue;
    }

    if (!connected) {
      mqtt_abort(&client);
      net_socket_service_unregister(&service_mqtt_dps);
    }
  }

  if (connected) {
    return 0;
  }

  return -EINVAL;
}

int korra_cloud_provisioning_clear() {
  int ret = settings_delete(SETTINGS_NAME_HOSTNAME);
  if (ret) {
    LOG_ERR("Unable to delete " SETTINGS_NAME_HOSTNAME " to settings");
    return ret;
  }

  settings_delete(SETTINGS_NAME_DEVICEID);
  if (ret) {
    LOG_ERR("Unable to delete " SETTINGS_NAME_DEVICEID " to settings");
    return ret;
  }

  return 0;
}

static int client_init(struct mqtt_client *client) {
  struct sockaddr broker_addr;
  socklen_t broker_addrlen;
  int ret = korra_resolve_address(DPS_HOSTNAME, 8883, AF_INET, SOCK_STREAM, &broker_addr, &broker_addrlen);
  if (ret) {
    LOG_ERR("Failed to resolve address: %d", ret);
    return ret;
  }
  memcpy(&broker, &broker_addr, sizeof(broker)); // at this time sockaddr_storage is the same as sockaddr

  mqtt_client_init(client);
  client->broker = &broker;
  client->evt_cb = mqtt_evt_handler;

  client->client_id.utf8 = clientid;
  client->client_id.size = clientid_len;
  client->user_name = &mqtt_username;
  client->password = NULL;
  client->protocol_version = MQTT_VERSION_3_1_1;
  client->rx_buf = rx_buffer;
  client->rx_buf_size = sizeof(rx_buffer);
  client->tx_buf = tx_buffer;
  client->tx_buf_size = sizeof(tx_buffer);
  client->clean_session = 1; // must be true (1) for DPS
  client->keepalive = 30;    // 30 seconds (default 60 seconds)

  client->transport.type = MQTT_TRANSPORT_SECURE;
  client->transport.tls.config.peer_verify = TLS_PEER_VERIFY_REQUIRED;
  client->transport.tls.config.cipher_list = NULL;
  client->transport.tls.config.sec_tag_list = m_sec_tags;
  client->transport.tls.config.sec_tag_count = ARRAY_SIZE(m_sec_tags);
  client->transport.tls.config.hostname = DPS_HOSTNAME;
  client->transport.tls.config.cert_nocopy = TLS_CERT_NOCOPY_OPTIONAL;

  return 0;
}

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt) {
  LOG_DBG("MQTT event: %d", evt->type);
  switch (evt->type) {
  case MQTT_EVT_CONNACK: {
    if (evt->result != 0) {
      LOG_ERR("MQTT connect failed %d", evt->result);
      break;
    }

    connected = true;
    LOG_INF("MQTT client connected!");
    break;
  }
  }
}

static void mqtt_service_handler(struct net_socket_service_event *pev) {
  LOG_DBG("MQTT service event: %d", pev->event.fd);
  mqtt_input(&client);
}

static int provisioning_info_save(const struct korra_cloud_provisioning_info *info) {
  int ret = settings_save_one(SETTINGS_NAME_HOSTNAME, info->hostname, info->hostname_len);
  if (ret) {
    LOG_ERR("Unable to save " SETTINGS_NAME_HOSTNAME " to settings");
    return ret;
  }

  settings_save_one(SETTINGS_NAME_DEVICEID, info->id, info->id_len);
  if (ret) {
    LOG_ERR("Unable to save " SETTINGS_NAME_DEVICEID " to settings");
    return ret;
  }

  return 0;
}
static void provisioning_info_print(const struct korra_cloud_provisioning_info *info) {
  LOG_INF("Valid: %s", info->valid ? "yes" : "no");
  LOG_INF("Hostname: %s (%d bytes)", info->hostname_len > 0 ? info->hostname : "[unset]", info->hostname_len);
  LOG_INF("Id: %s (%d bytes)", info->id_len > 0 ? info->id : "[unset]", info->id_len);
}
