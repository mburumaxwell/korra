#include <app_version.h>
#include <sys/reboot.h>

#include <SimpleSerialShell.h>

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
static bool update_device_twin(void *);
static void device_twin_updated(struct korra_device_twin *twin, bool initial);

static int shell_command_info(int argc, char **argv);
static int shell_command_reboot(int argc, char **argv);
static int shell_command_internet_cred_clear(int argc, char **argv);
static int shell_command_wifi_cred_set_open(int argc, char **argv);
static int shell_command_wifi_cred_set_personal(int argc, char **argv);
static int shell_command_wifi_cred_set_ent(int argc, char **argv);

void setup()
{
  delay(min(1, CONFIG_INITIAL_BOOT_DELAY_SECONDS) * 1000);

  uint64_t raw_devid = ESP.getEfuseMac();
  devid_len = snprintf(devid, sizeof(devid), "%llx", (unsigned long long)raw_devid);

  Serial.begin(9600);
  Serial.printf("*** Booting Korra %s build v%s (%s) ***\n", CONFIG_APP_NAME, APP_VERSION_STRING, STRINGIFY(APP_BUILD_VERSION));
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
  timing.begin(6 * 3600 /* 6 hours, in seconds */);

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
  hub.onDeviceTwinUpdated(device_twin_updated);

  // setup timers
  timer.every(500, maintain);
  timer.every((CONFIG_SENSORS_READ_PERIOD_SECONDS * 1000), collect_data);
  timer.every((3600 * 1000) /* 1 hour, in millis */, update_device_twin);

  // // clear credentials and/or provisioning info (only during test)
  // credentials.clear();
  // provisioning.clear();
  // internet.credentials_clear();
  // while (1)
  //   ;

  shell.attach(Serial);
  shell.addCommand(F("info"), shell_command_info);
  shell.addCommand(F("reboot"), shell_command_reboot);
  shell.addCommand(F("internet-cred-clear"), shell_command_internet_cred_clear);
  shell.addCommand(F("wifi-cred-set-open <ssid>"), shell_command_wifi_cred_set_open);
  shell.addCommand(F("wifi-cred-set-personal <ssid> <passphrase>"), shell_command_wifi_cred_set_personal);
  shell.addCommand(F("wifi-cred-set-ent <ssid> <identity> <username> <password>"), shell_command_wifi_cred_set_ent);
}

void loop()
{
  timer.tick();
  shell.executeIfInput();
}

static bool maintain(void *)
{
  internet.maintain();
  if (!internet.connected())
  {
    return true; // true to repeat the action, false to stop
  }

  mdns.maintain(internet.props());
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

  // check if hub is connected
  if (!hub.connected())
  {
    Serial.println("Hub is not connected. Skipping push.");
    return true; // true to repeat the action, false to stop
  }

  hub.push(&sensors_data, internet.props());
  return true; // true to repeat the action, false to stop
}

static bool update_device_twin(void *)
{
  // check if hub is connected
  if (!hub.connected())
  {
    return true; // true to repeat the action, false to stop
  }

  // check if the values we have reported need updating
  bool update = false;
  struct korra_device_twin_reported props = {0};
  memcpy(&props, &(hub.device_twin()->reported), sizeof(struct korra_device_twin_reported));

  // check firmware version (value)
  if (props.firmware.version.value != APP_VERSION_NUMBER)
  {
    props.firmware.version.value = APP_VERSION_NUMBER;
    update |= true;
  }

  // check firmware version (semver)
  if (strcmp(props.firmware.version.semver, APP_VERSION_STRING) != 0)
  {
    snprintf((char *)props.firmware.version.semver, sizeof(props.firmware.version.semver), APP_VERSION_STRING);
    update |= true;
  }

  // push the update to the hub
  if (update)
    hub.update(&props);
  else
    Serial.println("No update required for the reported properties in the device twin");

  return true; // true to repeat the action, false to stop
}

static void device_twin_updated(struct korra_device_twin *twin, bool initial)
{
  // for the first time, trigger an update in 5 seconds (it will check if there needs to be a push)
  if (initial)
  {
    timer.in((5 * 1000) /* 5 seconds, in millis */, [](void *) -> bool
             {
               update_device_twin(NULL);
               return false; // true to repeat the action, false to stop
             });
    return;
  }

  // check for firmware updates
  if (twin->desired.firmware.version.value != APP_VERSION_NUMBER ||
      strcmp(twin->desired.firmware.version.semver, APP_VERSION_STRING) != 0)
  {
    Serial.printf("We have a new firmware version: %s (%d)",
                  twin->desired.firmware.version.semver,
                  twin->desired.firmware.version.value);

    // TODO: handle new firmware
  }
}

static int shell_command_info(int argc, char **argv) // info
{
  Serial.printf("Korra %s build v%s (%s)\n", CONFIG_APP_NAME, APP_VERSION_STRING, STRINGIFY(APP_BUILD_VERSION));
  Serial.printf("Device ID: %s\n", devid);
  Serial.printf("Arduino Stack: %d of %d bytes free\n",
                uxTaskGetStackHighWaterMark(NULL),
                getArduinoLoopTaskStackSize());

  return EXIT_SUCCESS;
}

static int shell_command_reboot(int argc, char **argv) // reboot
{
  sys_reboot();
  return EXIT_SUCCESS;
}

static int shell_command_internet_cred_clear(int argc, char **argv) // internet-cred-clear
{
  Serial.println("Clearing internet credentials");
  return internet.credentials_clear() ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int shell_command_wifi_cred_set_open(int argc, char **argv) // wifi-cred-set-open <ssid>
{
  struct wifi_credentials credentials = {0};
  snprintf(credentials.ssid, sizeof(credentials.ssid), (char *)argv[1]);
  Serial.printf("Setting internet credentials (open network). SSID: '%s'\n", credentials.ssid);
  if (!internet.credentials_clear())
    return EXIT_FAILURE;
  return internet.credentials_save_wifi(&credentials) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int shell_command_wifi_cred_set_personal(int argc, char **argv) // wifi-cred-set-personal <ssid> <passphrase>
{
  struct wifi_credentials credentials = {0};
  snprintf(credentials.ssid, sizeof(credentials.ssid), (char *)argv[1]);
  snprintf(credentials.passphrase, sizeof(credentials.passphrase), (char *)argv[2]);
  Serial.printf("Setting internet credentials (personal). SSID: '%s'\n", credentials.ssid);
  if (!internet.credentials_clear())
    return EXIT_FAILURE;
  return internet.credentials_save_wifi(&credentials) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int shell_command_wifi_cred_set_ent(int argc, char **argv) // wifi-cred-set-ent <ssid> <identity> <username> <password>
{
  struct wifi_credentials credentials = {0};
  snprintf(credentials.ssid, sizeof(credentials.ssid), (char *)argv[1]);
  snprintf(credentials.eap_identity, sizeof(credentials.eap_identity), (char *)argv[2]);
  snprintf(credentials.eap_username, sizeof(credentials.eap_username), (char *)argv[3]);
  snprintf(credentials.eap_password, sizeof(credentials.eap_password), (char *)argv[4]);
  Serial.printf("Setting internet credentials (enterprise). SSID: '%s'\n", credentials.ssid);
  if (!internet.credentials_clear())
    return EXIT_FAILURE;
  return internet.credentials_save_wifi(&credentials) ? EXIT_SUCCESS : EXIT_FAILURE;
}
