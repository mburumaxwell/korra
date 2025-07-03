#ifndef KORRA_CLOUD_HUB_H_
#define KORRA_CLOUD_HUB_H_

#include "actuator/korra_actuator.h"
#include "korra_config.h"
#include "sensors/korra_sensors.h"

#ifdef CONFIG_BOARD_HAS_INTERNET

#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>

#include "internet/korra_network_shared.h"
#include "korra_cloud_shared.h"

struct korra_device_twin_firmware_version {
  uint32_t value;
  char semver[20 + 1]; // human-readable version
};

struct korra_device_twin_desired_firmware {
  struct korra_device_twin_firmware_version version; // version
  char url[256 + 1];                                 // firmware binary URL
  char hash[64 + 1];                                 // SHA-256 hash in hex
  char signature[128 + 1];                           // Signature (e.g., base64 or hex)
};

struct korra_device_twin_desired {
  uint32_t version; // $version
  struct korra_device_twin_desired_firmware firmware;
  struct korra_actuator_config actuator;
};

struct korra_device_twin_reported_firmware {
  struct korra_device_twin_firmware_version version; // version
};

struct korra_device_twin_reported {
  uint32_t version; // $version
  struct korra_device_twin_reported_firmware firmware;
  struct korra_actuator_state actuator;
};

struct korra_device_twin {
  struct korra_device_twin_desired desired;
  struct korra_device_twin_reported reported;
};

/**
 * This class is a wrapper for the cloud functionalities.
 */
class KorraCloudHub {
public:
  /**
   * Creates a new instance of the KorraCloudHub class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param client The secure TCP client to use for communication.
   */
  KorraCloudHub(Client &client);

  /**
   * Cleanup resources created and managed by the KorraCloudHub class.
   */
  ~KorraCloudHub();

  /**
   * Initializes the cloud hub logic.
   * This should be called once at the beginning of the program.
   */
  void begin();

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   *
   * @param info The device provisioning information.
   */
  void maintain(struct korra_cloud_provisioning_info *info);

  /**
   * Publishes data for configured sensors.
   * If the connection has not been established, nothing is published.
   *
   * @param source All values for configured sensors.
   * @param net_props Properties of the current network.
   */
  void push(const struct korra_sensors_data *source, const struct korra_network_props *net_props);

  /**
   * Update reported properties of the device twin.
   *
   * @param props The properties to be reported.
   */
  void update(struct korra_device_twin_reported *props);

  /**
   * Check if the connection to the cloud is established.
   *
   * @return `true` if the connection is established, `false` otherwise.
   */
  inline bool connected() { return mqtt.connected(); }

  /**
   * Disconnect the client from the cloud.
   */
  inline void disconnect() { mqtt.stop(); }

  /**
   * Get the device twin.
   */
  inline const struct korra_device_twin *device_twin() { return &twin; }

  /**
   * Registers callback that will be called each time the twin is updated.
   *
   * @param callback The callback to register.
   */
  inline void onDeviceTwinUpdated(void (*callback)(struct korra_device_twin *twin, bool initial)) {
    device_twin_updated_callback = callback;
  }

  /**
   * Returns existing instance (singleton) of the KorraCloudHub class.
   * It may be a null pointer if the KorraCloudHub object was never constructed or it was destroyed.
   */
  inline static KorraCloudHub *instance() { return _instance; }

  /**
   * Please do not call this method from outside the `KorraCloudProvisioning` class
   */
  void on_mqtt_message(int size);

private:
  /// Living instance of the KorraCloudHub class. It can be NULL.
  static KorraCloudHub *_instance;

private:
  void connect(int retries = 3, int delay_ms = 5000);
  void query_device_twin();
  void populate_desired_props(const JsonVariantConst &json, struct korra_device_twin_desired *desired);
  void populate_reported_props(const JsonVariantConst &json, struct korra_device_twin_reported *reported);

private:
  MqttClient mqtt;
  bool client_setup = false;
  char *username = NULL, *hostname = NULL, *deviceid = NULL;
  size_t username_len = 0, hostname_len = 0, deviceid_len = 0;
  uint16_t request_id = 1;
  bool twin_requested = false;
  struct korra_device_twin twin = {0};
  void (*device_twin_updated_callback)(struct korra_device_twin *twin, bool initial);
};

#endif // BOARD_HAS_INTERNET

#endif // KORRA_CLOUD_HUB_H_
