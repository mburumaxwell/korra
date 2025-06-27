#include <app_version.h>

#include "sensors/korra_sensors.h"

#include "internet/korra_internet.h"
#include "mdns/korra_mdns.h"
#include "time/korra_time.h"

KorraSensors sensors;
KorraSensorsData sensorsData;

KorraWifi wifi;
WiFiUDP udpClient;
KorraMdns mdns(udpClient);
KorraTime timing(udpClient);

void setup()
{
  Serial.begin(9600);
	Serial.printf("Booting Korra %s build v%s (%s)\n", CONFIG_APP_NAME, APP_VERSION_STRING, APP_BUILD_VERSION);

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
  // no need to set the reference voltage on ESP32-S3 because it offers reads in millivolts
  analogReadResolution(12); // change to 12-bit resolution

  sensors.begin();
  wifi.begin();
  mdns.begin(wifi.localIP(), wifi.hostname(), wifi.macAddress());
  timing.begin();
}

static uint32_t timepoint = millis();

void loop()
{
  if ((millis() - timepoint) >= (CONFIG_SENSORS_READ_PERIOD_SECONDS * 1000))
  {
    timepoint = millis();

    struct tm tm;
    timing.now(&tm);
    char time_str[sizeof("1970-01-01T00:00:00")];
    strftime(time_str, sizeof(time_str), "%FT%T", &tm);
    Serial.printf("Current ISO8601 time is %s\n", time_str);

    // TODO: restore this
    // sensors.read(&sensorsData);
  }

  wifi.maintain();
  mdns.maintain();
  timing.maintain();

  // This delay should be short so that the networking stuff is maintained correctly.
  // Network maintenance includes checking for WiFi connection, server connection, and sending PINGs.
  // Updating of sensor values happens over a longer delay by checking elapsed time above.
  delay(500);
}

void beforeReset()
{
  // save configuration and/or state so that we can resume after reboot
}
