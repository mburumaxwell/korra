#include "korra_cloud_hub.h"

// Username format -> {broker}/{device_id}/api-version=2018-06-30
#define USERNAME_FORMAT "%s/%s/api-version=2018-06-30"

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
        // prepare username as per spec
        username_len = snprintf(NULL, 0, USERNAME_FORMAT, info->hostname, info->id) + 1; // plus NULL
        username = (char *)malloc(username_len);
        if (username == NULL)
        {
            Serial.printf("Unable to allocate %d bytes for username\n", username_len);
            while (1)
                ;
        }
        username_len = snprintf(username, username_len, USERNAME_FORMAT, info->hostname, info->id);

        mqtt.setId(info->id);
        mqtt.setCleanSession(true);
        mqtt.setTxPayloadSize(1024);
        mqtt.setUsernamePassword(username, "" /* password (library throws when NULL) */);
        mqtt.setKeepAliveInterval(30 * 1000); // 30 seconds
        mqtt.onMessage(on_mqtt_message_callback);
        client_setup = true;
    }

    if (!connected())
    {
        connect(info->hostname);

        // if now connected, subscribe to topics
        if (connected())
        {
            // TODO: subscribe to relevant topics
        }
    }

    if (!connected())
    {
        return;
    }

    mqtt.poll();
}

void KorraCloudHub::push(KorraSensorsData *source)
{
    // mqtt.publish()
}

void KorraCloudHub::connect(const char *hostname, int retries, int delay_ms)
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

    // TODO: complete this
}
