#include <app_version.h>
#include <esp_system.h>

#include <SimpleSerialShell.h>

#include "sensors/korra_sensors.h"

#include "actuator/korra_actuator.h"
#include "cloud/korra_cloud.h"
#include "credentials/korra_credentials.h"
#include "internet/korra_internet.h"
#include "mdns/korra_mdns.h"
#include "ota/korra_ota.h"
#include "time/korra_time.h"

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

static KorraOta ota;

static struct korra_sensors_data sensors_data;
static char devid[(sizeof(uint64_t) * 2) + 1]; // the efuse is a 64-bit integer (64 bit -> 8 bytes -> 16 hex chars)
static size_t devid_len;

KorraActuator actuator;

static bool maintain(void *);
static bool collect_data(void *);
static bool maintain_ota(void *);
static bool reboot_timer(void *);
static bool update_device_twin(void *);
static void device_twin_updated(struct korra_device_twin *twin, bool initial);
static int device_direct_method_invoked(const char *method_name, const JsonVariantConst &payload);

static int shell_command_info(int argc, char **argv);
static int shell_command_reboot(int argc, char **argv);
static int shell_command_prefs_clear(int argc, char **argv);
static int shell_command_device_cred_clear(int argc, char **argv);
static int shell_command_provisioning_clear(int argc, char **argv);
static int shell_command_internet_cred_clear(int argc, char **argv);
static int shell_command_wifi_cred_set_open(int argc, char **argv);
static int shell_command_wifi_cred_set_personal(int argc, char **argv);
static int shell_command_wifi_cred_set_ent(int argc, char **argv);
static int shell_command_actuator_reset_state(int argc, char **argv);

void setup() {
  delay(3000); // allow time for the serial monitor to connect

  uint64_t raw_devid = ESP.getEfuseMac();
  devid_len = snprintf(devid, sizeof(devid), "%llx", (unsigned long long)raw_devid);

  Serial.begin(9600);
  Serial.printf("*** Running on ESP-IDF %s ***\n", esp_get_idf_version());
  Serial.printf("*** Booting Korra %s build v%s (%s) ***\n", CONFIG_APP_NAME, APP_VERSION_STRING,
                STRINGIFY(APP_BUILD_VERSION));
  Serial.printf("*** Device ID: %s ***\n", devid);

  if (!prefs.begin("korra", /* readonly */ false)) {
    Serial.println("Could not initialize preferences :-(");
    while (true);
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
  hub.onDirectMethodInvoked(device_direct_method_invoked);

  // setup OTA
  ota.begin(root_ca_certs);

  // setup actuator
  actuator.begin();
  actuator.onStateUpdated([](const struct korra_actuator_state *) { update_device_twin(NULL); });

  // setup timers
  timer.every(500, maintain);
  timer.every((CONFIG_SENSORS_READ_PERIOD_SECONDS * 1000), collect_data);
  timer.every((3600 * 1000) /* 1 hour, in millis */, update_device_twin);
  timer.every(1000, maintain_ota);
  timer.every(24 * 60 * 60 * 1000, reboot_timer); // reboot every 24 hours to address potential memory leaks and
                                                  // resource exhaustion observed during long uptime

  // setup shell
  shell.attach(Serial);
  shell.addCommand(F("info"), shell_command_info);
  shell.addCommand(F("reboot"), shell_command_reboot);
  shell.addCommand(F("prefs-clear"), shell_command_prefs_clear);
  shell.addCommand(F("device-cred-clear"), shell_command_device_cred_clear);
  shell.addCommand(F("provisioning-clear"), shell_command_provisioning_clear);
  shell.addCommand(F("internet-cred-clear"), shell_command_internet_cred_clear);
  shell.addCommand(F("wifi-cred-set-open <ssid>"), shell_command_wifi_cred_set_open);
  shell.addCommand(F("wifi-cred-set-personal <ssid> <passphrase>"), shell_command_wifi_cred_set_personal);
  shell.addCommand(F("wifi-cred-set-ent <ssid> <identity> <username> <password>"), shell_command_wifi_cred_set_ent);
  shell.addCommand(F("actuator-reset-state"), shell_command_actuator_reset_state);
}

void loop() {
  timer.tick();
  shell.executeIfInput();
}

static bool maintain(void *) {
  internet.maintain();
  if (!internet.connected()) return true; // true to repeat the action, false to stop

  mdns.maintain(internet.props());
  timing.maintain();

  // cloud maintenance
  provisioning.maintain();
  struct korra_cloud_provisioning_info *pi = provisioning.info();
  if (!pi->valid) return true; // true to repeat the action, false to stop
  hub.maintain(pi);

  // actuator maintenance
  actuator.maintain();

  return true; // true to repeat the action, false to stop
}

static bool collect_data(void *) {
  // read sensors data
  sensors.read(&sensors_data);

  // check if hub is connected
  if (!hub.connected()) {
    Serial.println("Hub is not connected. Skipping push.");
    return true; // true to repeat the action, false to stop
  }

  hub.push(&sensors_data);        // update the hub
  actuator.update(&sensors_data); // update the actuator

  return true; // true to repeat the action, false to stop
}

static bool maintain_ota(void *) {
  ota.maintain();
  return true; // true to repeat the action, false to stop
}

static bool reboot_timer(void *) {
  esp_restart();
}

static bool update_device_twin(void *) {
  // check if hub is connected
  if (!hub.connected()) return true; // true to repeat the action, false to stop

  // prepare the props to report
  struct korra_device_twin_reported props = {0};

  // set the firmware version (value and semver)
  props.firmware.version.value = APP_VERSION_NUMBER;
  snprintf((char *)props.firmware.version.semver, sizeof(props.firmware.version.semver), APP_VERSION_STRING);

  // set the network props
  memcpy(&(props.network), internet.props(), sizeof(struct korra_network_props));

  // set the actuator state
  memcpy(&(props.actuator), actuator.state(), sizeof(struct korra_actuator_state));

  // push the update to the hub
  hub.update(&props);

  return true; // true to repeat the action, false to stop
}

static void device_twin_updated(struct korra_device_twin *twin, bool initial) {
  // set values in the actuator
  actuator.set_config(&(twin->desired.actuator));

  // check for firmware updates
  if ((twin->desired.firmware.version.value && twin->desired.firmware.version.value != APP_VERSION_NUMBER)) {
    Serial.printf("We have a new firmware version: %s (%d)\n", twin->desired.firmware.version.semver,
                  twin->desired.firmware.version.value);

    // initialize the firmware update
    struct korra_ota_info ota_inf = {0};
    ota.populate(twin->desired.firmware.url, twin->desired.firmware.hash, twin->desired.firmware.signature, &ota_inf);
    ota.update(&ota_inf);
    return;
  }

  // for the first time, trigger an update in 5 seconds (it will check if there needs to be a push)
  if (initial) {
    // update in-memory state so that we continue from where we left off
    actuator.set_state(&(twin->reported.actuator));

    // update twin in 5 seconds (should set properties of what we are currently running)
    timer.in((5 * 1000) /* 5 seconds, in millis */, [](void *) -> bool {
      update_device_twin(NULL);
      return false; // true to repeat the action, false to stop
    });
  }
}

static int device_direct_method_invoked(const char *method_name, const JsonVariantConst &payload) {
  if (strcmp(method_name, "reboot") == 0) {
    Serial.println("Scheduling device reboot in 10 sec as requested by the cloud.");
    timer.in(10 * 1000, [](void *) -> bool {
      Serial.println("Rebooting device as requested by the cloud.");
      esp_restart();
      return false; // true to repeat the action, false to stop
    });
    return 200; // method executed successfully
  }
  return 404; // method not found
}

static int shell_command_info(int argc, char **argv) // info
{
  Serial.printf("Korra %s build v%s (%s)\n", CONFIG_APP_NAME, APP_VERSION_STRING, STRINGIFY(APP_BUILD_VERSION));
  Serial.printf("Device ID: %s\n", devid);
  Serial.printf("Arduino Stack: %d of %d bytes free\n", uxTaskGetStackHighWaterMark(NULL),
                getArduinoLoopTaskStackSize());

  return EXIT_SUCCESS;
}

static int shell_command_reboot(int argc, char **argv) {
  // command format: reboot

  esp_restart();
}

static int shell_command_prefs_clear(int argc, char **argv) {
  // command format: prefs-clear

  prefs.clear(); // clear preferences

  // reboot after 10 sec
  Serial.println("Rebooting in 10 sec ...");
  delay(10 * 1000);
  esp_restart();
}

static int shell_command_device_cred_clear(int argc, char **argv) {
  // command format: device-cred-clear

  hub.disconnect(); // disconnect from the cloud
  provisioning.disconnect();
  provisioning.clear(); // clear provisioning info
  credentials.clear();  // clear device cred

  // reboot after 10 sec
  Serial.println("Rebooting in 10 sec ...");
  delay(10 * 1000);
  esp_restart();
}

static int shell_command_provisioning_clear(int argc, char **argv) {
  // command format: provisioning-cred-clear

  hub.disconnect(); // disconnect from the cloud
  provisioning.disconnect();
  provisioning.clear(); // clear provisioning info

  // reboot after 5 sec
  Serial.println("It is often wise to reboot/reset after clearing provisioning info");

  return EXIT_SUCCESS;
}

static int shell_command_internet_cred_clear(int argc, char **argv) {
  // command format: internet-cred-clear
  Serial.println("Clearing internet credentials");
  return internet.credentials_clear() ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int shell_command_wifi_cred_set_open(int argc, char **argv) {
  // command format: wifi-cred-set-open <ssid>
  struct wifi_credentials credentials = {0};
  snprintf(credentials.ssid, sizeof(credentials.ssid), (char *)argv[1]);
  Serial.printf("Setting internet credentials (open network). SSID: '%s'\n", credentials.ssid);
  if (!internet.credentials_clear()) return EXIT_FAILURE;
  return internet.credentials_save_wifi(&credentials) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int shell_command_wifi_cred_set_personal(int argc, char **argv) {
  // command format: wifi-cred-set-personal <ssid> <passphrase>
  struct wifi_credentials credentials = {0};
  snprintf(credentials.ssid, sizeof(credentials.ssid), (char *)argv[1]);
  snprintf(credentials.passphrase, sizeof(credentials.passphrase), (char *)argv[2]);
  Serial.printf("Setting internet credentials (personal). SSID: '%s'\n", credentials.ssid);
  if (!internet.credentials_clear()) return EXIT_FAILURE;
  return internet.credentials_save_wifi(&credentials) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int shell_command_wifi_cred_set_ent(int argc, char **argv) {
  // command format: wifi-cred-set-ent <ssid> <identity> <username> <password>
  struct wifi_credentials credentials = {0};
  snprintf(credentials.ssid, sizeof(credentials.ssid), (char *)argv[1]);
  snprintf(credentials.eap_identity, sizeof(credentials.eap_identity), (char *)argv[2]);
  snprintf(credentials.eap_username, sizeof(credentials.eap_username), (char *)argv[3]);
  snprintf(credentials.eap_password, sizeof(credentials.eap_password), (char *)argv[4]);
  Serial.printf("Setting internet credentials (enterprise). SSID: '%s'\n", credentials.ssid);
  if (!internet.credentials_clear()) return EXIT_FAILURE;
  return internet.credentials_save_wifi(&credentials) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int shell_command_actuator_reset_state(int argc, char **argv) {
  // command format: actuator-reset-state
  struct korra_actuator_state state = {0};
  actuator.set_state(&state);
  update_device_twin(NULL);
  return EXIT_SUCCESS;
}
