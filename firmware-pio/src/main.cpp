#include <app_version.h>

#include "sensors/korra_sensors.h"

#include "credentials/korra_credentials.h"
#include "internet/korra_internet.h"
#include "mdns/korra_mdns.h"
#include "time/korra_time.h"
#include "cloud/korra_cloud.h"

static Preferences prefs;

static KorraSensors sensors;
static KorraCredentials credentials(prefs);
static KorraInternet internet(prefs);

static WiFiUDP udp_client;
static KorraMdns mdns(udp_client);
static KorraTime timing(udp_client);

static Timer<> timer;
static WiFiClientSecure tcp_client_provisioning;
static KorraCloudProvisioning provisioning(tcp_client_provisioning, prefs, timer);

static WiFiClientSecure tcp_client_hub; // each client can only open one socket so we cannot share
static KorraCloudHub hub(tcp_client_hub);

static struct korra_sensors_data sensors_data;
static char devid[(sizeof(uint64_t) * 2) + 1]; // the efuse is a 64-bit integer (64 bit -> 8 bytes -> 16 hex chars)
static size_t devid_len;

static bool maintain(void *);
static bool collect_data(void *);

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
  internet.begin();
  mdns.begin(internet.props());
  timing.begin(6 * 3600 /* 6 hours, in seconds */);

  // force early sync so that credentials and everything else in the system works easy
  timing.sync();

  // prepare credentials
  credentials.begin(devid, devid_len);
  const char *root_ca_certs = credentials.root_ca_certs();
  const char *devcert = credentials.device_cert();
  const char *devkey = credentials.device_key();
  Serial.printf("Device certificate to use for provisioning:\n%s\n", devcert);

  // setup cloud (provisioning)
  tcp_client_provisioning.setCACert(root_ca_certs);
  tcp_client_provisioning.setCertificate(devcert);
  tcp_client_provisioning.setPrivateKey(devkey);
  provisioning.begin(devid, devid_len);

  // setup cloud (hub)
  tcp_client_hub.setCACert(root_ca_certs);
  tcp_client_hub.setCertificate(devcert);
  tcp_client_hub.setPrivateKey(devkey);
  hub.begin();

  // setup timers
  timer.every(500, maintain);
  timer.every((CONFIG_SENSORS_READ_PERIOD_SECONDS * 1000), collect_data);

  // // clear credentials and/or provisioning info (only during test)
  // credentials.clear();
  // provisioning.clear();
}

static bool maintain(void *)
{
  internet.maintain();
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
  // read sensors data
  sensors.read(&sensors_data);

  // check if hubs is connected
  if (!hub.connected())
  {
    Serial.println("Hub is not connected. Skipping push.");
    return true; // true to repeat the action, false to stop
  }

  hub.push(&sensors_data, internet.props());
  return true; // true to repeat the action, false to stop
}

void loop()
{
  timer.tick();
}
