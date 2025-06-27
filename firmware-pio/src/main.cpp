#include <app_version.h>

#include "sensors/korra_sensors.h"

#include "credentials/korra_credentials.h"
#include "internet/korra_internet.h"
#include "mdns/korra_mdns.h"
#include "time/korra_time.h"

Preferences prefs;

KorraSensors sensors;
KorraSensorsData sensorsData;

KorraCredentials credentials(prefs);
KorraWifi wifi;

WiFiUDP udpClient;
WiFiClientSecure tcpClient;

KorraMdns mdns(udpClient);
KorraTime timing(udpClient);

char devid[(sizeof(uint64_t) * 2) + 1]; // the efuse is a 64-bit integer (64 bit -> 8 bytes -> 16 hex chars)
size_t devid_len;

void setup()
{
#if defined(CONFIG_INITIAL_BOOT_DELAY_SECONDS) && CONFIG_INITIAL_BOOT_DELAY_SECONDS > 1
  delay(CONFIG_INITIAL_BOOT_DELAY_SECONDS * 1000);
#endif // CONFIG_INITIAL_BOOT_DELAY_SECONDS

  uint64_t raw_devid = ESP.getEfuseMac();
  devid_len = snprintf(devid, sizeof(devid), "%llx", (unsigned long long)raw_devid);

  Serial.begin(9600);
	Serial.printf("*** Booting Korra %s build v%s (%s) ***\n", CONFIG_APP_NAME, APP_VERSION_STRING, APP_BUILD_VERSION);
	Serial.printf("*** Device ID: %s ***\n", devid);

  if (!prefs.begin("korra", /* readonly */ false))
  {
    while (true)
    {
      Serial.println("Could not initialize preferences :-(");
      delay(60 * 1000);
    }
  }

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
  // no need to set the reference voltage on ESP32-S3 because it offers reads in millivolts
  analogReadResolution(12); // change to 12-bit resolution

  sensors.begin();

  // setup setupworking
  wifi.begin();
  mdns.begin(wifi.localIP(), wifi.hostname(), wifi.macAddress());
  timing.begin();

  // force early sync so that credentials and everything else in the system works easy
  timing.sync();

  // prepare credentials
  credentials.begin(devid, devid_len);
  const char *devcert = credentials.device_cert();
  Serial.printf("Device certificate to use for provisioning:\n%s\n", devcert);
  tcpClient.setCACert(credentials.root_ca_certs());
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
  timing.sync();

  // This delay should be short so that the networking stuff is maintained correctly.
  // Network maintenance includes checking for WiFi connection, server connection, and sending PINGs.
  // Updating of sensor values happens over a longer delay by checking elapsed time above.
  delay(500);
}

void beforeReset()
{
  // save configuration and/or state so that we can resume after reboot
}
