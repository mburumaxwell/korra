#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_cloud_provisioning, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/settings/settings.h>

#include <korra_credentials.h>
#include <korra_net_utils.h>

#include "korra_cloud_provisioning.h"

// official/ref docs
// - https://learn.microsoft.com/en-us/azure/iot/iot-mqtt-connect-to-iot-dps

// Username format -> {idScope}/registrations/{registration_id}/api-version=2019-03-31
#define USERNAME_FORMAT "%s/registrations/%s/api-version=2019-03-31"

#define SETTINGS_NAME_HOSTNAME "azure-hub"
#define SETTINGS_NAME_DEVICEID "azure-deviceid"
#define DPS_HOSTNAME "global.azure-devices-provisioning.net"
#define DPS_ID_SCOPE CONFIG_AZURE_IOT_DPS_ID_SCOPE
#define DPS_MQTT_PORT 8883

static const sec_tag_t m_sec_tags[] = {
    KORRA_CREDENTIAL_AZURE_CA_TAG,
    KORRA_CREDENTIAL_DEVICE_TAG,
};

static uint8_t rx_buffer[256];
static uint8_t tx_buffer[256];
static struct mqtt_client client;
static struct sockaddr_storage broker;

static char *clientid, *username;
static size_t clientid_len, username_len;
// static int dps_provision(void);
// static int iot_hub_connect(void);

static struct korra_cloud_provisioning_info *info;

static int provisioning_info_save();
static void provisioning_info_print();
static int client_init();
static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);

int korra_cloud_provisioning_init(const char *regid, const size_t regid_len,
                                  struct korra_cloud_provisioning_info *result) {
  LOG_DBG("Initializing");
  clientid = (char *)regid;
  clientid_len = regid_len;

  memset(result, 0, sizeof(struct korra_cloud_provisioning_info));
  info = result;

  // load hub hostname
  ssize_t ret_or_len = settings_load_one(SETTINGS_NAME_HOSTNAME, result->hostname, sizeof(result->hostname));
  if (ret_or_len < 0) {
    LOG_ERR("Unable to load " SETTINGS_NAME_HOSTNAME " from settings: %d", ret_or_len);
    return ret_or_len;
  }
  result->hostname_len = ret_or_len;

  ret_or_len = settings_load_one(SETTINGS_NAME_DEVICEID, result->id, sizeof(result->id));
  if (ret_or_len) {
    LOG_ERR("Unable to load " SETTINGS_NAME_DEVICEID " from settings: %d", ret_or_len);
    return ret_or_len;
  }
  result->id_len = ret_or_len;

  result->valid = result->hostname_len > 0 && result->id_len > 0;
  provisioning_info_print(result);

  // prepare username as per spec
  username_len = snprintf(NULL, 0, USERNAME_FORMAT, DPS_ID_SCOPE, regid) + 1; // plus NULL
  username = k_malloc(username_len);
  if (username == NULL) {
    LOG_ERR("Unable to allocate %d bytes for username", username_len);
    return -ENOMEM;
  }
  username_len = snprintf(username, username_len, USERNAME_FORMAT, DPS_ID_SCOPE, regid);

  return 0;
}

int korra_cloud_provisioning_run() {
  client_init();

  return 0;
}

static int client_init() {
  struct sockaddr broker_addr;
  socklen_t broker_addrlen;
  int ret = korra_resolve_address(DPS_HOSTNAME, DPS_MQTT_PORT, AF_INET, SOCK_STREAM, &broker_addr, &broker_addrlen);
  if (ret) {
    LOG_ERR("Failed to resolve address: %d", ret);
    return ret;
  }
  memcpy(&broker, &broker_addr, sizeof(broker)); // at this time sockaddr_storage is the same as sockaddr

  struct mqtt_utf8 mqtt_username = {.utf8 = username, .size = username_len};

  mqtt_client_init(&client);
  client.broker = &broker;
  client.evt_cb = mqtt_evt_handler;

  client.client_id.utf8 = clientid;
  client.client_id.size = clientid_len;
  client.user_name = &mqtt_username;
  client.protocol_version = MQTT_VERSION_3_1_1;
  client.rx_buf = rx_buffer;
  client.rx_buf_size = sizeof(rx_buffer);
  client.tx_buf = tx_buffer;
  client.tx_buf_size = sizeof(tx_buffer);
  client.clean_session = 1; // must be true (1) for DPS
  client.keepalive = 30;    // 30 seconds (default 60 seconds)

  client.transport.type = MQTT_TRANSPORT_SECURE;
  client.transport.tls.config.peer_verify = TLS_PEER_VERIFY_REQUIRED;
  client.transport.tls.config.cipher_list = NULL;
  client.transport.tls.config.sec_tag_list = m_sec_tags;
  client.transport.tls.config.sec_tag_count = ARRAY_SIZE(m_sec_tags);
  client.transport.tls.config.hostname = DPS_HOSTNAME;

  return 0;
}

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt) {
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

static int provisioning_info_save() {
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
static void provisioning_info_print() {
  LOG_DBG("Valid: %s", info->valid ? "yes" : "no");
  LOG_DBG("Hostname: %s (%d bytes)", info->hostname_len > 0 ? info->hostname : "[unset]", info->hostname_len);
  LOG_DBG("Id: %s (%d bytes)", info->id_len > 0 ? info->id : "[unset]", info->id_len);
}
