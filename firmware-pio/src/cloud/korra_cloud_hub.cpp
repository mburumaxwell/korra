#include <ArduinoJson.h>
#include <sys/reboot.h>

#include "korra_cloud_hub.h"

// Username format -> {iotHub-hostname}/{device_id}/api-version=2021-04-12
#define USERNAME_FORMAT "%s/%s/api-version=2021-04-12"

// Device to Cloud message topic -> devices/{device-id}/messages/events/
//                               -> devices/{device-id}/messages/events/{property-bag}
#define TOPIC_FORMAT_D2C_MESSAGE "devices/%s/messages/events/"

// Cloud to device message topic filter -> devices/{device-id}/messages/devicebound/#
#define TOPIC_C2D_PREFIX "devices/%s/messages/devicebound/"
#define TOPIC_C2D_FILTER TOPIC_C2D_PREFIX "#"

#define TOPIC_TWIN_RESULT_PREFIX "$iothub/twin/res/"
#define TOPIC_TWIN_RESULT_FILTER TOPIC_TWIN_RESULT_PREFIX "#"
#define TOPIC_FORMAT_TWIN_GET_STATUS "$iothub/twin/GET/?$rid=%d"
#define TOPIC_FORMAT_TWIN_PATCH_REPORTED "$iothub/twin/PATCH/properties/reported/?$rid=%d"

KorraCloudHub *KorraCloudHub::_instance = nullptr;

static void on_mqtt_message_callback(int size) {
  KorraCloudHub::instance()->on_mqtt_message(size);
}

KorraCloudHub::KorraCloudHub(Client &client) : mqtt(client) {
  _instance = this;
}

KorraCloudHub::~KorraCloudHub() {
  _instance = nullptr;

  if (username) {
    free(username);
    username = NULL;
  }
  username_len = 0;
}

void KorraCloudHub::begin() {
}

void KorraCloudHub::maintain(struct korra_cloud_provisioning_info *info) {
  if (!client_setup) {
    // set fields
    hostname = info->hostname;
    hostname_len = info->hostname_len;
    deviceid = info->id;
    deviceid_len = info->id_len;

    // prepare username as per spec
    username_len = snprintf(NULL, 0, USERNAME_FORMAT, hostname, deviceid) + 1; // plus NULL
    username = (char *)malloc(username_len);
    if (username == NULL) {
      Serial.printf("Unable to allocate %d bytes for username\n", username_len);
      while (1);
    }
    username_len = snprintf(username, username_len, USERNAME_FORMAT, hostname, deviceid);

    mqtt.setId(deviceid);
    mqtt.setTxPayloadSize(512);            // defaults to 256
    mqtt.setCleanSession(false);           // want to received any messages we missed
    mqtt.setKeepAliveInterval(240 * 1000); // 240 seconds (default 60 seconds)
    mqtt.setUsernamePassword(username, "" /* password (library throws when NULL) */);
    mqtt.onMessage(on_mqtt_message_callback);
    client_setup = true;
  }

  if (!connected()) {
    connect();

    // if now connected, subscribe to topics
    if (connected()) {
      // subscribe to inbound/C2D messages
      size_t topic_len = snprintf(NULL, 0, TOPIC_C2D_FILTER, deviceid);
      char topic[topic_len + 1] = {0};
      topic_len = snprintf(topic, sizeof(topic), TOPIC_C2D_FILTER, deviceid);
      mqtt.subscribe(topic, /* qos */ 0);

      // subscribe to device twin messages
      mqtt.subscribe(TOPIC_TWIN_RESULT_FILTER, /* qos */ 0);
    }
  }

  if (!connected()) return;

  mqtt.poll();

  // request twin if not requested
  if (!twin_requested) {
    query_device_twin();
    twin_requested = true;
  }
}

void KorraCloudHub::push(const struct korra_sensors_data *source, const struct korra_network_props *net_props) {
  JsonDocument doc;

  // network telemetry should ideally go to twin reported properties but for now this is easier.
  doc["network"]["kind"] = net_props->kind;
  doc["network"]["mac"] = net_props->mac;
  doc["network"]["network"] = net_props->network;
  doc["network"]["local_ip"] = net_props->local_ip;

  // set timestamp (though it exists in the properties of the message, this ensures it is also in the body)
  time_t now = time(NULL);
  doc["timestamp"] = now;

  // set the IOS8601 version of the timestamp
  struct tm tm;
  gmtime_r(&now, &tm);
  char time_str[sizeof("1970-01-01T00:00:00")];
  strftime(time_str, sizeof(time_str), "%FT%T", &tm);
  doc["created"] = time_str;

#ifdef CONFIG_APP_KIND_KEEPER
  doc["temperature"]["unit"] = "C";
  doc["temperature"]["value"] = source->temperature;
  doc["humidity"]["unit"] = "%";
  doc["humidity"]["value"] = source->humidity;
#endif // CONFIG_APP_KIND_KEEPER
#ifdef CONFIG_APP_KIND_POT
  doc["moisture"]["unit"] = "%";
  doc["moisture"]["value"] = source->moisture;
  doc["ph"]["value"] = source->ph;
#endif // CONFIG_APP_KIND_POT

  // prepare topic
  size_t topic_len = snprintf(NULL, 0, TOPIC_FORMAT_D2C_MESSAGE, deviceid);
  char topic[topic_len + 1] = {0};
  topic_len = snprintf(topic, sizeof(topic), TOPIC_FORMAT_D2C_MESSAGE, deviceid);

  // prepare payload
  size_t payload_len = measureJson(doc);
  char payload[payload_len + 1] = {0};
  payload_len = serializeJson(doc, payload, payload_len);
  Serial.printf("Sending message to topic '%s', length %d bytes:\n%s\n", topic, payload_len, payload);

  // publish
  mqtt.beginMessage(topic, /* retain */ false, /* qos */ 0, /* dup */ false);
  mqtt.write((const uint8_t *)payload, payload_len);
  mqtt.endMessage();
}

void KorraCloudHub::update(struct korra_device_twin_reported *props) {
  JsonDocument doc;

  // The request message body contains a JSON document that contains new values for reported properties.
  // Each member in the JSON document updates or add the corresponding member in the device twin's document.
  // A member set to null deletes the member from the containing object.
  doc["firmware"]["version"]["value"] = props->firmware.version.value;
  doc["firmware"]["version"]["semver"] = props->firmware.version.semver;
  doc["actuator"]["count"] = props->actuator.count;
  doc["actuator"]["last_time"] = props->actuator.last_time;
  doc["actuator"]["total_duration"] = props->actuator.total_duration;

  // prepare topic
  size_t topic_len = snprintf(NULL, 0, TOPIC_FORMAT_TWIN_PATCH_REPORTED, request_id);
  char topic[topic_len + 1] = {0};
  topic_len = snprintf(topic, sizeof(topic), TOPIC_FORMAT_TWIN_PATCH_REPORTED, request_id);

  // prepare payload
  size_t payload_len = measureJson(doc);
  char payload[payload_len + 1] = {0};
  payload_len = serializeJson(doc, payload, payload_len);
  Serial.printf("Sending message to topic '%s', length %d bytes:\n%s\n", topic, payload_len, payload);

  // publish
  mqtt.beginMessage(topic, /* retain */ false, /* qos */ 0, /* dup */ false);
  mqtt.write((const uint8_t *)payload, payload_len);
  mqtt.endMessage();
  request_id++;
  memcpy(&(twin.reported.firmware), props, sizeof(struct korra_device_twin_reported));
}

void KorraCloudHub::connect(int retries, int delay_ms) {
  for (int i = 0; i < retries; i++) {
    // connect with retries
    Serial.printf("Connecting to Hub server... (%d/%d)\n", i + 1, retries);
    mqtt.connect(hostname, 8883);
    if (connected()) {
      Serial.println("Connected to Hub server.");
      break;
    }
    Serial.printf("Failed to connect to Hub server (%d). Retrying in %d ms...\n", mqtt.connectError(), delay_ms);
    delay(delay_ms);
  }

  if (!connected()) {
    Serial.printf("Failed to connect to Hub server after %d retries of %d ms each.\n", retries, delay_ms);
  }
}

void KorraCloudHub::query_device_twin() {
  Serial.printf("Requesting device twin with rid: %d\n", request_id);
  size_t topic_len = snprintf(NULL, 0, TOPIC_FORMAT_TWIN_GET_STATUS, request_id);
  char topic[topic_len + 1] = {0};
  topic_len = snprintf(topic, sizeof(topic), TOPIC_FORMAT_TWIN_GET_STATUS, request_id);
  mqtt.beginMessage(topic, /* retain */ false, /* qos */ 0, /* dup */ false);
  mqtt.print("{}"); // must be an empty json otherwise it won't work
  mqtt.endMessage();
  request_id++;
}

void KorraCloudHub::on_mqtt_message(int size) {
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
  // twin -> $iothub/registrations/res/200/?$rid=1

  // find the prefix
  const char *prefix_pos = strstr(topic, TOPIC_TWIN_RESULT_PREFIX);
  if (prefix_pos == NULL) {
    Serial.printf("Unknown topic. Expected prefix: %s\n", TOPIC_TWIN_RESULT_PREFIX);
    return;
  }
  int status_code = 0;
  sscanf(prefix_pos + strlen(TOPIC_TWIN_RESULT_PREFIX), "%d", &status_code);

  if (status_code == 200) {
    // parse the json payload
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    const uint16_t desired_version = doc["desired"]["$version"];
    const uint16_t reported_version = doc["reported"]["$version"];
    const bool changed = desired_version != twin.desired.version || reported_version != twin.reported.version;
    const bool initial = twin.desired.version == 0 || twin.reported.version == 0;
    if (!changed) {
      // no changes, nothing to do
      return;
    }

    // reset the stored twin
    twin = {0};

    // update the twin stored locally (desired)
    twin.desired.version = desired_version;
    // desired.firmware.version.value
    twin.desired.firmware.version.value = doc["desired"]["firmware"]["version"]["value"];
    // desired.firmware.version.semver
    const char *semver_raw = doc["desired"]["firmware"]["version"]["semver"];
    if (semver_raw != NULL) {
      size_t semver_raw_len = min((int)strlen(semver_raw) + 1, (int)sizeof(twin.desired.firmware.version.semver));
      memcpy(twin.desired.firmware.version.semver, semver_raw, semver_raw_len - 1);
    }
    // desired.firmware.url
    const char *url_raw = doc["desired"]["firmware"]["url"];
    if (url_raw != NULL) {
      size_t url_raw_len = min((int)strlen(url_raw) + 1, (int)sizeof(twin.desired.firmware.url));
      memcpy(twin.desired.firmware.url, url_raw, url_raw_len - 1);
    }
    // desired.firmware.hash
    const char *hash_raw = doc["desired"]["firmware"]["hash"];
    if (hash_raw != NULL) {
      size_t hash_raw_len = min((int)strlen(hash_raw) + 1, (int)sizeof(twin.desired.firmware.hash));
      memcpy(twin.desired.firmware.hash, hash_raw, hash_raw_len - 1);
    }
    // desired.firmware.signature
    const char *signature_raw = doc["desired"]["firmware"]["signature"];
    if (signature_raw != NULL) {
      size_t signature_raw_len = min((int)strlen(signature_raw) + 1, (int)sizeof(twin.desired.firmware.signature));
      memcpy(twin.desired.firmware.signature, signature_raw, signature_raw_len - 1);
    }
    // desired.actuator.enabled
    twin.desired.actuator.enabled = doc["desired"]["actuator"]["enabled"];
    // desired.actuator.target
    twin.desired.actuator.target = doc["desired"]["actuator"]["target"];

    // update the twin stored locally (reported)
    twin.reported.version = reported_version;
    // reported.firmware.version.value
    twin.reported.firmware.version.value = doc["reported"]["firmware"]["version"]["value"];
    // reported.firmware.version.semver
    semver_raw = doc["reported"]["firmware"]["version"]["semver"];
    if (semver_raw != NULL) {
      size_t semver_raw_len = min((int)strlen(semver_raw) + 1, (int)sizeof(twin.reported.firmware.version.semver));
      memcpy(twin.reported.firmware.version.semver, semver_raw, semver_raw_len - 1);
    }

    // invoke callback
    if (changed && device_twin_updated_callback != NULL) {
      device_twin_updated_callback(&twin, initial);
    }
  } else if (status_code == 204) {
    Serial.println("Update was successful.");
  } else {
    // assume a transient error and reboot (should actually parse the error)
    Serial.println("Unknown status code. Rebooting in 5 seconds ...");
    delay(5000);
    sys_reboot();
  }
}
