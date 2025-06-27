#include <app_version.h>

#include "sensors/korra_sensors.h"

#include "credentials/korra_credentials.h"
#include "internet/korra_internet.h"
#include "mdns/korra_mdns.h"
#include "time/korra_time.h"
#include "cloud/korra_cloud.h"

struct KorraSensorsData sensors_data;

Preferences prefs;

Timer<> timer;
static bool maintain(void *);
static bool collect_data(void *);

KorraSensors sensors;
KorraCredentials credentials(prefs);
KorraWifi wifi;

WiFiUDP udp_client;
KorraMdns mdns(udp_client);
KorraTime timing(udp_client);

WiFiClientSecure tcp_client_provisioning;
KorraCloudProvisioning provisioning(tcp_client_provisioning, prefs, timer);

WiFiClientSecure tcp_client_hub; // each client can only open one socket so we cannot share
KorraCloudHub hub(tcp_client_hub);

char devid[(sizeof(uint64_t) * 2) + 1]; // the efuse is a 64-bit integer (64 bit -> 8 bytes -> 16 hex chars)
size_t devid_len;

void setup()
{
  delay(min(1, CONFIG_INITIAL_BOOT_DELAY_SECONDS) * 1000);

  uint64_t raw_devid = ESP.getEfuseMac();
  devid_len = snprintf(devid, sizeof(devid), "%llx", (unsigned long long)raw_devid);

  Serial.begin(9600);
  Serial.printf("*** Booting Korra %s build v%s (%s) ***\n", CONFIG_APP_NAME, APP_VERSION_STRING, APP_BUILD_VERSION);
  Serial.printf("*** Device ID: %s ***\n", devid);

  if (!prefs.begin("korra", /* readonly */ false))
  {
    Serial.println("Could not initialize preferences :-(");
    while (true)
      ;
  }

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
  // no need to set the reference voltage on ESP32-S3 because it offers reads in millivolts
  analogReadResolution(12); // change to 12-bit resolution

  sensors.begin();

  // setup networking
  wifi.begin();
  mdns.begin(wifi.localIP(), wifi.hostname(), wifi.macAddress());
  timing.begin();

  // force early sync so that credentials and everything else in the system works easy
  timing.sync();

  // prepare credentials
  credentials.begin(devid, devid_len);
  const char *devcert = credentials.device_cert();
  Serial.printf("Device certificate to use for provisioning:\n%s\n", devcert);

  // setup cloud (provisioning)
  provisioning.begin(devid, devid_len);
  tcp_client_provisioning.setCACert(credentials.root_ca_certs());
  tcp_client_provisioning.setCertificate(devcert);
  tcp_client_provisioning.setPrivateKey(credentials.device_key());

  // setup cloud (hub)
  tcp_client_hub.setCACert(credentials.root_ca_certs());
  tcp_client_hub.setCertificate(devcert);
  tcp_client_hub.setPrivateKey(credentials.device_key());
  hub.begin();

  // setup timers
  timer.every(500, maintain);
  timer.every((CONFIG_SENSORS_READ_PERIOD_SECONDS * 1000), collect_data);

  // clear credentials and/or provisioning info (only during test)
  // credentials.clear();
  // provisioning.clear();
}

static bool maintain(void *)
{
  wifi.maintain();
  mdns.maintain();
  timing.maintain();

  // cloud maintenance
  provisioning.maintain();
  struct korra_cloud_provisioning_info *pi = provisioning.info();
  if (!pi->valid)
  {
    return true; // true to repeat the action, false to stop
  }
  hub.maintain(pi);

  return true; // true to repeat the action, false to stop
}

static bool collect_data(void *)
{
  // TODO: restore this
  // sensors.read(&sensors_data);

  struct korra_cloud_provisioning_info *pi = provisioning.info();
  if (!pi->valid)
  {
    Serial.println("Provisioning info is not valid. Skipping push.");
    return true; // true to repeat the action, false to stop
  }

  // TODO: restore this
  hub.push(&sensors_data);
  return true; // true to repeat the action, false to stop
}

void loop()
{
  timer.tick();
}

void beforeReset()
{
  // save configuration and/or state so that we can resume after reboot
}
