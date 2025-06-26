#include "internet/korra_internet.h"
#include "mdns/korra_mdns.h"
#include "sensors/korra_sensors.h"

KorraSensors sensors;
KorraSensorsData sensorsData;

KorraWifi wifi;
WiFiUDP udpClient;
KorraMdns mdns(udpClient);

void setup()
{
  Serial.begin(9600);

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
  // no need to set the reference voltage on ESP32-S3 because it offers reads in millivolts
  analogReadResolution(12); // change to 12-bit resolution

  sensors.begin();
  wifi.begin();
  mdns.begin(wifi.localIP(), wifi.hostname(), wifi.macAddress());
}

static uint32_t timepoint = millis();

void loop()
{
  if ((millis() - timepoint) >= (CONFIG_SENSORS_READ_PERIOD_SECONDS * 1000))
  {
    timepoint = millis();
    // TODO: restore this
    // sensors.read(&sensorsData);
  }

  wifi.maintain();
  mdns.maintain();

  // This delay should be short so that the networking stuff is maintained correctly.
  // Network maintenance includes checking for WiFi connection, server connection, and sending PINGs.
  // Updating of sensor values happens over a longer delay by checking elapsed time above.
  delay(500);
}

void beforeReset()
{
  // save configuration and/or state so that we can resume after reboot
}
