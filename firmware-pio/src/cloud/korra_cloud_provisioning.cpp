#include <ArduinoJson.h>

#include "korra_cloud_provisioning.h"

#define PREFERENCES_KEY_HOSTNAME "azure-hub"
#define PREFERENCES_KEY_DEVICEID "azure-deviceid"
#define DPS_HOSTNAME "global.azure-devices-provisioning.net"

// Username format -> {idScope}/registrations/{registration_id}/api-version=2019-03-31
#define USERNAME_FORMAT "%s/registrations/%s/api-version=2019-03-31"

#define TOPIC_FORMAT_REGISTER "$dps/registrations/PUT/iotdps-register/?$rid=%d"
#define TOPIC_FORMAT_GET_STATUS "$dps/registrations/GET/iotdps-get-operationstatus/?$rid=%d&operationId=%s"

#define TOPIC_REGISTRATION_RESULT_PREFIX "$dps/registrations/res/"
#define TOPIC_REGISTRATION_RESULT_FILTER TOPIC_REGISTRATION_RESULT_PREFIX "#"
#define TOPIC_REGISTRATION_RESULT_RETRY_AFTER_KEY "retry-after="

KorraCloudProvisioning *KorraCloudProvisioning::_instance = NULL;

static void on_mqtt_message_callback(int size) {
  KorraCloudProvisioning::instance()->on_mqtt_message(size);
}

KorraCloudProvisioning::KorraCloudProvisioning(Client &client, Preferences &prefs, Timer<> &timer)
    : mqtt(client), prefs(prefs), timer(timer) {
  _instance = this;
}

KorraCloudProvisioning::~KorraCloudProvisioning() {
  _instance = NULL;

  if (username) {
    free(username);
    username = NULL;
  }
  username_len = 0;

  free_status();
}

void KorraCloudProvisioning::begin(const char *regid, const size_t regid_len) {
  // if we have valid info, there is nothing todo
  load();
  if (info()->valid) return;

  // prepare username as per spec
  username_len = snprintf(NULL, 0, USERNAME_FORMAT, CONFIG_AZURE_IOT_DPS_ID_SCOPE, regid) + 1; // plus NULL
  username = (char *)malloc(username_len);
  if (username == NULL) {
    Serial.printf("Unable to allocate %d bytes for username\n", username_len);
    while (1);
  }
  username_len = snprintf(username, username_len, USERNAME_FORMAT, CONFIG_AZURE_IOT_DPS_ID_SCOPE, regid);

  // setup the MQTT client
  mqtt.setId(regid);
  mqtt.setCleanSession(true);           // must be true (1) for DPS
  mqtt.setKeepAliveInterval(30 * 1000); // 30 seconds (default 60 seconds)
  mqtt.setUsernamePassword(username, "" /* password (library throws when NULL) */);
  mqtt.onMessage(on_mqtt_message_callback);
}

void KorraCloudProvisioning::maintain() {
  // if we have valid info, there is nothing todo
  if (info()->valid) return;

  if (!connected()) {
    registration_requested = false;
    connect();

    // if now connected, subscribe to topics
    if (connected()) {
      // subscribe to topic for registration results
      mqtt.subscribe(TOPIC_REGISTRATION_RESULT_FILTER, /* qos */ 0);
    }
  }

  if (!connected()) return;

  mqtt.poll();

  // request registration if not requested
  if (!registration_requested) {
    Serial.printf("Requesting DPS registration with rid: %d\n", request_id);
    size_t topic_len = snprintf(NULL, 0, TOPIC_FORMAT_REGISTER, request_id);
    char topic[topic_len + 1] = {0};
    topic_len = snprintf(topic, sizeof(topic), TOPIC_FORMAT_REGISTER, request_id);
    mqtt.beginMessage(topic, /* retain */ false, /* qos */ 0, /* dup */ false);
    mqtt.print("{}"); // must be an empty json otherwise it won't work
    mqtt.endMessage();
    request_id++;
    registration_requested = true;
  }
}

void KorraCloudProvisioning::print() {
  Serial.printf("Provisioning Info: Valid: %s\n", stored_info.valid ? "yes" : "no");
  Serial.printf("Provisioning Info: Hostname: %s (%d bytes)\n",
                stored_info.hostname_len > 0 ? stored_info.hostname : "[unset]", stored_info.hostname_len);
  Serial.printf("Provisioning Info: Id: %s (%d bytes)\n", stored_info.id_len > 0 ? stored_info.id : "[unset]",
                stored_info.id_len);
}

void KorraCloudProvisioning::load() {
  if (prefs.isKey(PREFERENCES_KEY_HOSTNAME)) {
    prefs.getBytes(PREFERENCES_KEY_HOSTNAME, stored_info.hostname, sizeof(stored_info.hostname));
    stored_info.hostname_len = prefs.getBytesLength(PREFERENCES_KEY_HOSTNAME);
  }

  if (prefs.isKey(PREFERENCES_KEY_DEVICEID)) {
    prefs.getBytes(PREFERENCES_KEY_DEVICEID, stored_info.id, sizeof(stored_info.id));
    stored_info.id_len = prefs.getBytesLength(PREFERENCES_KEY_DEVICEID);
  }

  stored_info.valid = stored_info.hostname_len > 0 && stored_info.id_len > 0;
  print();
}

void KorraCloudProvisioning::save() {
  prefs.putBytes(PREFERENCES_KEY_HOSTNAME, stored_info.hostname, stored_info.hostname_len);
  prefs.putBytes(PREFERENCES_KEY_DEVICEID, stored_info.id, stored_info.id_len);
}

void KorraCloudProvisioning::clear() {
  prefs.remove(PREFERENCES_KEY_HOSTNAME);
  prefs.remove(PREFERENCES_KEY_DEVICEID);
}

void KorraCloudProvisioning::connect(int retries, int delay_ms) {
  for (int i = 0; i < retries; i++) {
    // connect with retries
    Serial.printf("Connecting to DPS server... (%d/%d)\n", i + 1, retries);
    mqtt.connect(DPS_HOSTNAME, 8883);
    if (connected()) {
      Serial.println("Connected to DPS server.");
      break;
    }
    Serial.printf("Failed to connect to DPS server (%d). Retrying in %d ms...\n", mqtt.connectError(), delay_ms);
    delay(delay_ms);
  }

  if (!connected()) {
    Serial.printf("Failed to connect to DPS server after %d retries of %d ms each.\n", retries, delay_ms);
  }
}

void KorraCloudProvisioning::query_registration_result() {
  Serial.printf("Requesting DPS registration result for operationId='%s' and rid: %d\n", status.operation_id,
                request_id);
  size_t topic_len = snprintf(NULL, 0, TOPIC_FORMAT_GET_STATUS, request_id, status.operation_id);
  char topic[topic_len + 1] = {0};
  snprintf(topic, sizeof(topic), TOPIC_FORMAT_GET_STATUS, request_id, status.operation_id);
  mqtt.beginMessage(topic, /* retain */ false, /* qos */ 0, /* dup */ false);
  mqtt.print("{}"); // must be an empty json otherwise it won't work
  mqtt.endMessage();
}

void KorraCloudProvisioning::on_mqtt_message(int size) {
  // Keep the String alive until the function ends since we depend on it.
  // Converting direct with c_str(), discards the string but we need it.
  String topic_str = mqtt.messageTopic();
  const char *topic = topic_str.c_str();
  Serial.printf("Received a message on topic '%s', length %d bytes:\n", topic, size);

  // read payload
  char payload[size + 1] = {0};
  mqtt.readBytes(payload, size);
  Serial.printf("%s\n", payload);

  // sample topics
  // 1 (first      ) -> $dps/registrations/res/202/?$rid=1&retry-after=3
  // 2 (after query) -> $dps/registrations/res/200/?$rid={request_id}

  // find the prefix
  const char *prefix_pos = strstr(topic, TOPIC_REGISTRATION_RESULT_PREFIX);
  if (prefix_pos == NULL) {
    Serial.printf("Unknown topic. Expected prefix: %s\n", TOPIC_REGISTRATION_RESULT_PREFIX);
    return;
  }
  int status_code = 0;
  sscanf(prefix_pos + strlen(TOPIC_REGISTRATION_RESULT_PREFIX), "%d", &status_code);

  // find the query key
  const char *retry_pos = strstr(topic, TOPIC_REGISTRATION_RESULT_RETRY_AFTER_KEY);
  int retry_after = 0;
  if (retry_pos) {
    sscanf(retry_pos + strlen(TOPIC_REGISTRATION_RESULT_RETRY_AFTER_KEY), "%d", &retry_after);
  }

  // parse the json payload
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (status_code == 202) {
    // payload: {"operationId":"5.b583bfc3b016efc9.eb496f7a-d7cd-48fe-9f11-702b7552f44a","status":"assigning"}
    free_status();

    // operation id
    const char *operation_id_raw = doc["operationId"];
    size_t operation_id_raw_len = strlen(operation_id_raw) + 1;
    status.operation_id = (char *)malloc(operation_id_raw_len);
    memset(status.operation_id, 0, operation_id_raw_len);
    memcpy(status.operation_id, operation_id_raw, operation_id_raw_len - 1);

    // status
    status.status = status.registration_state.status = parse_status(doc["status"]);

    // schedule retry/query
    schedule_query_registration_result(retry_after);
  } else if (status_code == 200) {
    // {"operationId":"5.b583bfc3b016efc9.df67b89b-0e9a-4ff9-8a5f-1b5012341abd","status":"assigned","registrationState":{"x509":{},"registrationId":"471a1188534","createdDateTimeUtc":"2025-06-27T22:15:36.8196494Z","assignedHub":"korra.azure-devices.net","deviceId":"471a1188534","status":"assigned","substatus":"initialAssignment","lastUpdatedDateTimeUtc":"2025-06-27T22:15:36.949507Z","etag":"IjE4MDEzYzM2LTAwMDAtMTEwMC0wMDAwLTY4NWYxODA4MDAwMCI="}}
    free_status();

    // operation id
    const char *operation_id_raw = doc["operationId"];
    size_t operation_id_raw_len = strlen(operation_id_raw) + 1;
    status.operation_id = (char *)malloc(operation_id_raw_len);
    memset(status.operation_id, 0, operation_id_raw_len);
    memcpy(status.operation_id, operation_id_raw, operation_id_raw_len - 1);

    // status
    status.status = status.registration_state.status = parse_status(doc["status"]);
    if (status.status != KorraCloudProvisioning::PROVISIONING_REGISTRATION_STATUS_ASSIGNED) {
      Serial.printf("Status: %d is not assigned. Rescheduling ...\n", status.status);
      schedule_query_registration_result(8);
      return;
    }

    // assigned hub
    const char *assigned_hub_raw = doc["registrationState"]["assignedHub"];
    size_t assigned_hub_raw_len = strlen(assigned_hub_raw) + 1;
    status.registration_state.assigned_hub = (char *)malloc(assigned_hub_raw_len);
    memset(status.registration_state.assigned_hub, 0, assigned_hub_raw_len);
    memcpy(status.registration_state.assigned_hub, assigned_hub_raw, assigned_hub_raw_len - 1);

    // device id
    const char *device_id_raw = doc["registrationState"]["deviceId"];
    size_t device_id_raw_len = strlen(device_id_raw) + 1;
    status.registration_state.device_id = (char *)malloc(device_id_raw_len);
    memset(status.registration_state.device_id, 0, device_id_raw_len);
    memcpy(status.registration_state.device_id, device_id_raw, device_id_raw_len - 1);

    // save to info
    Serial.printf("Assigned '%s/%s'\n", status.registration_state.assigned_hub, status.registration_state.device_id);
    stored_info = {0};
    stored_info.hostname_len = assigned_hub_raw_len - 1;
    stored_info.id_len = device_id_raw_len - 1;
    memcpy(stored_info.hostname, status.registration_state.assigned_hub, stored_info.hostname_len);
    memcpy(stored_info.id, status.registration_state.device_id, stored_info.id_len);
    stored_info.valid = stored_info.hostname_len > 0 && stored_info.id_len > 0;
    save();

    // disconnect from the server
    mqtt.stop();
  } else {
    // it is likely a transient error, we should actually parse the error
    // TODO: parse the error

    // not rebooting to allow other tasks to continue
    Serial.println("Unknown status code. It is wise to reboot ...");
  }
}

KorraCloudProvisioning::provisioning_registration_status KorraCloudProvisioning::parse_status(const char *value) {
  if (strcasecmp(value, "unassigned") == 0) {
    return KorraCloudProvisioning::provisioning_registration_status::PROVISIONING_REGISTRATION_STATUS_UNASSIGNED;
  } else if (strcasecmp(value, "assigning") == 0) {
    return KorraCloudProvisioning::provisioning_registration_status::PROVISIONING_REGISTRATION_STATUS_ASSIGNING;
  } else if (strcasecmp(value, "assigned") == 0) {
    return KorraCloudProvisioning::provisioning_registration_status::PROVISIONING_REGISTRATION_STATUS_ASSIGNED;
  } else if (strcasecmp(value, "failed") == 0) {
    return KorraCloudProvisioning::provisioning_registration_status::PROVISIONING_REGISTRATION_STATUS_FAILED;
  } else if (strcasecmp(value, "disabled") == 0) {
    return KorraCloudProvisioning::provisioning_registration_status::PROVISIONING_REGISTRATION_STATUS_DISABLED;
  }
  return KorraCloudProvisioning::provisioning_registration_status::PROVISIONING_REGISTRATION_STATUS_UNKNOWN;
}

KorraCloudProvisioning::provisioning_registration_sub_status KorraCloudProvisioning::parse_sub_status(
    const char *value) {
  if (strcasecmp(value, "initialAssignment") == 0) {
    return KorraCloudProvisioning::provisioning_registration_sub_status::
        PROVISIONING_REGISTRATION_SUB_STATUS_INITIAL_ASSIGNMENT;
  } else if (strcasecmp(value, "deviceDataMigrated") == 0) {
    return KorraCloudProvisioning::provisioning_registration_sub_status::
        PROVISIONING_REGISTRATION_SUB_STATUS_DEVICE_DATA_MIGRATED;
  } else if (strcasecmp(value, "deviceDataReset") == 0) {
    return KorraCloudProvisioning::provisioning_registration_sub_status::
        PROVISIONING_REGISTRATION_SUB_STATUS_DEVICE_DATA_RESET;
  }
  return KorraCloudProvisioning::provisioning_registration_sub_status::PROVISIONING_REGISTRATION_SUB_STATUS_UNKNOWN;
}

void KorraCloudProvisioning::free_status() {
  char *values[] = {
      status.operation_id,
      status.registration_state.registration_id,
      status.registration_state.assigned_hub,
      status.registration_state.device_id,
      status.registration_state.generation_id,
      status.registration_state.error_message,
      status.registration_state.etag,
      status.registration_state.json_payload,
      status.retry_after,
  };

  for (uint8_t i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
    if (values[i]) {
      free(values[i]);
    }
  }

  status = {0};
}

void KorraCloudProvisioning::schedule_query_registration_result(int delay_sec) {
  delay_sec = MAX(3, delay_sec);
  Serial.printf("Scheduling DPS retry/query in %d sec\n", delay_sec);
  timer.in((delay_sec * 1000), [](void *) -> bool {
    KorraCloudProvisioning::instance()->query_registration_result();
    return false; // true to repeat the action, false to stop
  });
}
