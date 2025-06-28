#ifndef KORRA_CLOUD_HUB_H_
#define KORRA_CLOUD_HUB_H_

#include "korra_config.h"
#include "sensors/korra_sensors.h"

#ifdef CONFIG_BOARD_HAS_INTERNET

#include <ArduinoMqttClient.h>

#include "internet/korra_network_shared.h"
#include "korra_cloud_shared.h"

/**
 * This class is a wrapper for the cloud functionalities.
 */
class KorraCloudHub
{
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
   * Initializes the time logic.
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
   * Check if the connection to the cloud is established.
   *
   * @return `true` if the connection is established, `false` otherwise.
   */
  inline bool connected() { return mqtt.connected(); }

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
  /// Living instance of the KorraCloudHub class. It can be nullptr.
  static KorraCloudHub *_instance;

private:
  void connect(int retries = 3, int delay_ms = 5000);

private:
  MqttClient mqtt;
  bool client_setup = false;
  char *username = NULL, *hostname = NULL, *deviceid = NULL;
  size_t username_len = 0, hostname_len = 0, deviceid_len = 0;
};

#endif // BOARD_HAS_INTERNET

#endif // KORRA_CLOUD_HUB_H_
