#include <ArduinoJson.h>

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

KorraCloudHub *KorraCloudHub::_instance = nullptr;

static void on_mqtt_message_callback(int size)
{
    KorraCloudHub::instance()->on_mqtt_message(size);
}

KorraCloudHub::KorraCloudHub(Client &client) : mqtt(client)
{
    _instance = this;
}

KorraCloudHub::~KorraCloudHub()
{
    _instance = nullptr;

    if (username)
    {
        free(username);
        username = NULL;
    }
    username_len = 0;
}

void KorraCloudHub::begin()
{
}

void KorraCloudHub::maintain(struct korra_cloud_provisioning_info *info)
{
    if (!client_setup)
    {
        // set fields
        hostname = info->hostname;
        hostname_len = info->hostname_len;
        deviceid = info->id;
        deviceid_len = info->id_len;

        // prepare username as per spec
        username_len = snprintf(NULL, 0, USERNAME_FORMAT, hostname, deviceid) + 1; // plus NULL
        username = (char *)malloc(username_len);
        if (username == NULL)
        {
            Serial.printf("Unable to allocate %d bytes for username\n", username_len);
            while (1)
                ;
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

    if (!connected())
    {
        connect();

        // if now connected, subscribe to topics
        if (connected())
        {
            // subscribe to inbound/C2D messages
            size_t topic_len = snprintf(NULL, 0, TOPIC_C2D_FILTER, deviceid);
            char topic[topic_len + 1] = {0};
            topic_len = snprintf(topic, sizeof(topic), TOPIC_C2D_FILTER, deviceid);
            mqtt.subscribe(topic, /* qos */ 0);

            // subscribe to device twin messages
            mqtt.subscribe(TOPIC_TWIN_RESULT_FILTER, /* qos */ 0);
        }
    }

    if (!connected())
    {
        return;
    }

    mqtt.poll();
}

void KorraCloudHub::push(const struct korra_sensors_data *source, const struct korra_network_props *net_props)
{
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

void KorraCloudHub::connect(int retries, int delay_ms)
{
    for (int i = 0; i < retries; i++)
    {
        // connect with retries
        Serial.printf("Connecting to Hub server... (%d/%d)\n", i + 1, retries);
        mqtt.connect(hostname, 8883);
        if (connected())
        {
            Serial.println("Connected to Hub server.");
            break;
        }
        Serial.printf("Failed to connect to Hub server (%d). Retrying in %d ms...\n", mqtt.connectError(), delay_ms);
        delay(delay_ms);
    }

    if (!connected())
    {
        Serial.printf("Failed to connect to Hub server after %d retries of %d ms each.\n", retries, delay_ms);
    }
}

void KorraCloudHub::on_mqtt_message(int size)
{
    // Keep the String alive until the function ends since we depend on it.
    // Converting direct with c_str(), discards the string but we need it.
    String topic_str = mqtt.messageTopic();
    const char *topic = topic_str.c_str();
    Serial.printf("Received a message on topic '%s', length %d bytes:\n", topic, size);

    // read payload
    char payload[size + 1];
    mqtt.readBytes(payload, size);
    Serial.printf("%s\n", payload);
}
