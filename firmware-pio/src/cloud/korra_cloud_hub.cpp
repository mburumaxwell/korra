#include "korra_cloud_hub.h"

// Username format -> {iotHub-hostname}/{device_id}/api-version=2021-04-12
#define USERNAME_FORMAT "%s/%s/api-version=2021-04-12"

// Device to Cloud message topic -> devices/{device-id}/messages/events/
//                               -> devices/{device-id}/messages/events/{property-bag}
// System properties are added automatically and their keys are prefixed with "$."
// Application properties are added as key-value pairs in the property bag.
#define TOPIC_FORMAT_D2C_MESSAGE "devices/%s/messages/events/$.ct=application%%2Fjson%%3Bcharset%%3Dutf-8&type=%s"

// Cloud to device message topic filter -> devices/{device-id}/messages/devicebound/#
#define TOPIC_C2D_PREFIX "devices/%s/messages/devicebound/"
#define TOPIC_C2D_FILTER TOPIC_C2D_PREFIX "#"

// Direct method message topic filter -> $iothub/methods/POST/#
#define TOPIC_DIRECT_METHOD_PREFIX "$iothub/methods/POST/"
#define TOPIC_DIRECT_METHOD_FILTER TOPIC_DIRECT_METHOD_PREFIX "#"
// Direct method response topic format -> $iothub/methods/res/{status}/?$rid={request-id}
#define TOPIC_FORMAT_DIRECT_METHOD_RESPONSE "$iothub/methods/res/%d/?$rid=%d"

#define TOPIC_TWIN_RESULT_PREFIX "$iothub/twin/res/"
#define TOPIC_TWIN_RESULT_FILTER TOPIC_TWIN_RESULT_PREFIX "#"
#define TOPIC_FORMAT_TWIN_GET_STATUS "$iothub/twin/GET/?$rid=%d"
#define TOPIC_FORMAT_TWIN_PATCH_REPORTED "$iothub/twin/PATCH/properties/reported/?$rid=%d"
#define TOPIC_TWIN_PATCH_PREFIX "$iothub/twin/PATCH/properties/desired/"
#define TOPIC_TWIN_PATCH_FILTER TOPIC_TWIN_PATCH_PREFIX "#"

KorraCloudHub *KorraCloudHub::_instance = NULL;

static void on_mqtt_message_callback(int size) {
  KorraCloudHub::instance()->on_mqtt_message(size);
}

KorraCloudHub::KorraCloudHub(Client &client, Timer<> &timer) : mqtt(client), timer(timer) {
  _instance = this;
}

KorraCloudHub::~KorraCloudHub() {
  _instance = NULL;

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
      mqtt.subscribe(TOPIC_TWIN_RESULT_FILTER, /* qos */ 0);   // request response
      mqtt.subscribe(TOPIC_TWIN_PATCH_FILTER, /* qos */ 0);    // updates to desired props
      mqtt.subscribe(TOPIC_DIRECT_METHOD_FILTER, /* qos */ 0); // direct methods
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

void KorraCloudHub::push(const struct korra_sensors_data *source) {
  JsonDocument doc;

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
  doc["app_kind"] = "keeper";
  doc["temperature"]["unit"] = "C";
  doc["temperature"]["value"] = source->temperature;
  doc["humidity"]["unit"] = "%";
  doc["humidity"]["value"] = source->humidity;
#endif // CONFIG_APP_KIND_KEEPER
#ifdef CONFIG_APP_KIND_POT
  doc["app_kind"] = "pot";
  doc["moisture"]["unit"] = "%";
  doc["moisture"]["value"] = source->moisture.value;
  doc["moisture"]["millivolts"] = source->moisture.millivolts;
  doc["ph"]["value"] = source->ph.value;
  doc["ph"]["millivolts"] = source->ph.millivolts;
#endif // CONFIG_APP_KIND_POT

  // prepare topic
  size_t topic_len = snprintf(NULL, 0, TOPIC_FORMAT_D2C_MESSAGE, deviceid, "sensors");
  char topic[topic_len + 1] = {0};
  topic_len = snprintf(topic, sizeof(topic), TOPIC_FORMAT_D2C_MESSAGE, deviceid, "sensors");

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
  // The request message body contains a JSON document that contains new values for reported properties.
  // Each member in the JSON document updates or add the corresponding member in the device twin's document.
  // A member set to null deletes the member from the containing object.
  JsonDocument doc;

  // check if the values we have reported need updating then update
  bool update = false;

  // check the firmware version (either value or semver)
  if (props->firmware.version.value != twin.reported.firmware.version.value ||
      strcmp(props->firmware.version.semver, twin.reported.firmware.version.semver) != 0) {

    twin.reported.firmware.version.value = props->firmware.version.value;
    snprintf((char *)twin.reported.firmware.version.semver, sizeof(twin.reported.firmware.version.semver),
             props->firmware.version.semver);

    doc["firmware"]["version"]["value"] = props->firmware.version.value;
    doc["firmware"]["version"]["semver"] = props->firmware.version.semver;
    update |= true;
  }

  // check the network props
  if (strcmp(props->network.kind, twin.reported.network.kind) != 0) {
    snprintf((char *)twin.reported.network.kind, sizeof(twin.reported.network.kind), props->network.kind);
    doc["network"]["kind"] = props->network.kind;
    update |= true;
  }
  if (strcmp(props->network.mac, twin.reported.network.mac) != 0) {
    snprintf((char *)twin.reported.network.mac, sizeof(twin.reported.network.mac), props->network.mac);
    doc["network"]["mac"] = props->network.mac;
    update |= true;
  }
  if (strcmp(props->network.name, twin.reported.network.name) != 0) {
    snprintf((char *)twin.reported.network.name, sizeof(twin.reported.network.name), props->network.name);
    doc["network"]["name"] = props->network.name;
    update |= true;
  }
  if (strcmp(props->network.local_ip, twin.reported.network.local_ip) != 0) {
    snprintf((char *)twin.reported.network.local_ip, sizeof(twin.reported.network.local_ip), props->network.local_ip);
    doc["network"]["local_ip"] = props->network.local_ip;
    update |= true;
  }

  // check the actuator state
  if (props->actuator.count != twin.reported.actuator.count) {
    twin.reported.actuator.count = props->actuator.count;
    doc["actuator"]["count"] = props->actuator.count;
    update |= true;
  }
  if (props->actuator.last_time != twin.reported.actuator.last_time) {
    twin.reported.actuator.last_time = props->actuator.last_time;
    doc["actuator"]["last_time"] = props->actuator.last_time;
    update |= true;
  }
  if (props->actuator.total_duration != twin.reported.actuator.total_duration) {
    twin.reported.actuator.total_duration = props->actuator.total_duration;
    doc["actuator"]["total_duration"] = props->actuator.total_duration;
    update |= true;
  }

  // if we have nothing to update, return
  if (!update) {
    Serial.println("No update required for the reported properties in the device twin");
    return;
  }

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
  // twin (request response) -> $iothub/registrations/res/200/?$rid=1
  // twin (updated desired)  -> $iothub/twin/PATCH/properties/desired/?$version={new-version}
  // direct method call      -> $iothub/methods/POST/{method-name}/?$rid={request-id}

  // find the prefix (twin request response)
  const char *prefix_pos = strstr(topic, TOPIC_TWIN_RESULT_PREFIX);
  if (prefix_pos != NULL) {
    int status_code = 0;
    if (sscanf(prefix_pos + strlen(TOPIC_TWIN_RESULT_PREFIX), "%d", &status_code) != 1) {
      Serial.println(F("Error: Failed to parse status code."));
      return;
    }

    if (status_code == 200) {
      // parse the json payload
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      const auto node_desired = doc["desired"];
      const auto node_reported = doc["reported"];
      const uint16_t desired_version = node_desired["$version"].as<uint16_t>();
      const uint16_t reported_version = node_reported["$version"].as<uint16_t>();
      const bool changed = desired_version != twin.desired.version || reported_version != twin.reported.version;
      const bool initial = twin.desired.version == 0 || twin.reported.version == 0;
      if (!changed) {
        // no changes, nothing to do
        return;
      }

      // reset the stored twin and then populate it
      twin = {0};
      twin.desired.version = desired_version;
      twin.reported.version = reported_version;
      populate_desired_props(node_desired, &(twin.desired));
      populate_reported_props(node_reported, &(twin.reported));

      // invoke callback
      if (changed && device_twin_updated_callback != NULL) {
        device_twin_updated_callback(&twin, initial);
      }
    } else if (status_code == 204) {
      Serial.println("Update was successful.");
    } else {
      // it is likely a transient error, we should actually parse the error
      // TODO: parse the error

      // not rebooting to allow other tasks to continue
      Serial.println("Unknown status code. It is wise to reboot ...");
    }

    return;
  }

  // find the prefix (twin desired update)
  prefix_pos = strstr(topic, TOPIC_TWIN_PATCH_PREFIX);
  if (prefix_pos != NULL) {
    // parse the json payload
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // populate
    twin.desired.version = doc["$version"].as<uint16_t>();
    populate_desired_props(doc, &(twin.desired));

    // invoke callback
    if (device_twin_updated_callback != NULL) {
      device_twin_updated_callback(&twin, /* initial */ false);
    }

    return;
  }

  // find the prefix (direct method call)
  prefix_pos = strstr(topic, TOPIC_DIRECT_METHOD_PREFIX);
  if (prefix_pos != NULL) {
    char method_name[64] = {0};
    int rid = 0;
    if (sscanf(prefix_pos + strlen(TOPIC_DIRECT_METHOD_PREFIX), "%63[^/]/?$rid=%d", method_name, &rid) != 2) {
        Serial.println(F("Error: Failed to parse direct method call topic."));
        return;
    }
    Serial.printf("Direct method call: %s (RID: %d)\n", method_name, rid);

    // parse the json payload
    JsonDocument doc;
    if (size > 0) {
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
    }

    // invoke the callback
    int status_code = 404; // default status code (not found)
    if (direct_method_call_callback != NULL) {
      status_code = direct_method_call_callback(method_name, doc);
    }
    direct_method_response(status_code, rid);

    return;
  }

  Serial.printf("Unknown topic.\n");
}

void KorraCloudHub::populate_desired_props(const JsonVariantConst &json, struct korra_device_twin_desired *desired) {
  // firmware version
  JsonVariantConst node_fw = json["firmware"];
  if (!node_fw.isNull()) {
    // firmware version
    JsonVariantConst node_fw_v = node_fw["version"];
    if (!node_fw_v.isNull()) {
      twin.desired.firmware.version.value = node_fw_v["value"].as<uint32_t>();
      const char *semver_raw = node_fw_v["semver"];
      if (semver_raw != NULL) {
        size_t semver_raw_len = MIN((int)strlen(semver_raw) + 1, (int)sizeof(twin.desired.firmware.version.semver));
        memcpy(twin.desired.firmware.version.semver, semver_raw, semver_raw_len - 1);
      }
    }

    // firmware url
    const char *url_raw = node_fw["url"];
    if (url_raw != NULL) {
      size_t url_raw_len = MIN((int)strlen(url_raw) + 1, (int)sizeof(twin.desired.firmware.url));
      memcpy(twin.desired.firmware.url, url_raw, url_raw_len - 1);
    }

    // firmware hash
    const char *hash_raw = node_fw["hash"];
    if (hash_raw != NULL) {
      size_t hash_raw_len = MIN((int)strlen(hash_raw) + 1, (int)sizeof(twin.desired.firmware.hash));
      memcpy(twin.desired.firmware.hash, hash_raw, hash_raw_len - 1);
    }

    // firmware signature
    const char *signature_raw = node_fw["signature"];
    if (signature_raw != NULL) {
      size_t signature_raw_len = MIN((int)strlen(signature_raw) + 1, (int)sizeof(twin.desired.firmware.signature));
      memcpy(twin.desired.firmware.signature, signature_raw, signature_raw_len - 1);
    }
  }

  // actuator
  JsonVariantConst node_acc = json["actuator"];
  if (!node_acc.isNull()) {
    twin.desired.actuator.enabled = node_acc["enabled"].as<bool>();
    twin.desired.actuator.duration = node_acc["duration"].as<uint16_t>();
    twin.desired.actuator.equilibrium_time = node_acc["equilibrium_time"].as<uint16_t>();
    twin.desired.actuator.target = node_acc["target"].as<float>();

    // clamp actuator values
    twin.desired.actuator.duration = CLAMP(twin.desired.actuator.duration, 5, 15);
    twin.desired.actuator.equilibrium_time = CLAMP(twin.desired.actuator.equilibrium_time, 5, 60);
  }
}

void KorraCloudHub::populate_reported_props(const JsonVariantConst &json, struct korra_device_twin_reported *reported) {
  // firmware version
  JsonVariantConst node_fv = json["firmware"]["version"];
  if (!node_fv.isNull()) {
    twin.reported.firmware.version.value = node_fv["value"].as<uint32_t>();
    const char *semver_raw = node_fv["semver"];
    if (semver_raw != NULL) {
      size_t semver_raw_len = MIN((int)strlen(semver_raw) + 1, (int)sizeof(twin.reported.firmware.version.semver));
      memcpy(twin.reported.firmware.version.semver, semver_raw, semver_raw_len - 1);
    }
  }

  // network
  JsonVariantConst node_net = json["network"];
  if (!node_net.isNull()) {
    const char *kind_raw = node_net["kind"];
    if (kind_raw != NULL) {
      size_t kind_raw_len = MIN((int)strlen(kind_raw) + 1, (int)sizeof(twin.reported.network.kind));
      memcpy(twin.reported.network.kind, kind_raw, kind_raw_len - 1);
    }
    const char *mac_raw = node_net["mac"];
    if (mac_raw != NULL) {
      size_t mac_raw_len = MIN((int)strlen(mac_raw) + 1, (int)sizeof(twin.reported.network.mac));
      memcpy(twin.reported.network.mac, mac_raw, mac_raw_len - 1);
    }
    const char *name_raw = node_net["name"];
    if (name_raw != NULL) {
      size_t name_raw_len = MIN((int)strlen(name_raw) + 1, (int)sizeof(twin.reported.network.name));
      memcpy(twin.reported.network.name, name_raw, name_raw_len - 1);
    }
    const char *local_ip_raw = node_net["local_ip"];
    if (local_ip_raw != NULL) {
      size_t local_ip_raw_len = MIN((int)strlen(local_ip_raw) + 1, (int)sizeof(twin.reported.network.local_ip));
      memcpy(twin.reported.network.local_ip, local_ip_raw, local_ip_raw_len - 1);
    }
  }

  // actuator
  JsonVariantConst node_acc = json["actuator"];
  if (!node_acc.isNull()) {
    twin.reported.actuator.count = node_acc["count"].as<uint16_t>();
    twin.reported.actuator.last_time = node_acc["last_time"].as<time_t>();
    twin.reported.actuator.total_duration = node_acc["total_duration"].as<uint32_t>();
  }
}

void KorraCloudHub::direct_method_response(int status_code, int request_id) {
  // prepare topic
  size_t topic_len = snprintf(NULL, 0, TOPIC_FORMAT_DIRECT_METHOD_RESPONSE, status_code, request_id);
  char topic[topic_len + 1] = {0};
  snprintf(topic, sizeof(topic), TOPIC_FORMAT_DIRECT_METHOD_RESPONSE, status_code, request_id);
  Serial.printf("Sending message to topic '%s'\n", topic);

  // publish
  mqtt.beginMessage(topic, /* retain */ false, /* qos */ 0, /* dup */ false);
  mqtt.print("{}"); // must be an empty json otherwise it won't work
  mqtt.endMessage();
}
