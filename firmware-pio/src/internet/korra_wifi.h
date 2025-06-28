#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "korra_config.h"
#include "korra_network_shared.h"

#ifdef CONFIG_BOARD_HAS_WIFI

#include <WiFi.h>
#include <WiFiClientSecure.h>

/**
 * This class is a wrapper for the WiFi logic.
 * It is where all the WiFi related code is located.
 */
class KorraWifi
{
public:
  /**
   * Creates a new instance of the KorraWifi class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  KorraWifi();

  /**
   * Cleanup resources created and managed by the KorraWifi class.
   */
  ~KorraWifi();

  /**
   * Scan for networks and connect to the one configured.
   */
  void begin();

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

  /**
   * Get the props of the network.
   */
  inline const struct korra_network_props *props() { return &net_props; }

private:
  uint8_t _status;
  korra_network_props net_props;

private:
#ifdef CONFIG_WIFI_SCAN_NETWORKS
  void listNetworks();
#endif // CONFIG_WIFI_SCAN_NETWORKS

  void connect(bool initial);
};

#endif // BOARD_HAS_WIFI

#endif // WIFI_MANAGER_H
