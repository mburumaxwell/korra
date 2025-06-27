#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "korra_config.h"

#if BOARD_HAS_WIFI

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
   * Get the station interface IP address.
   */
  inline IPAddress localIP() { return WiFi.localIP(); }

  /**
   * Get the station interface IP address.
   */
  inline uint8_t *macAddress() { return _macAddress; }

  /**
   * Get the station interface IP address.
   */
  inline const char *hostname() { return _hostname; }

private:
  uint8_t _status;
  char _hostname[19]; // e.g. "korra-012345678901" plus EOF
  uint8_t _macAddress[MAC_ADDRESS_LENGTH];

private:
#if !WIFI_SKIP_LIST_NETWORKS
  void listNetworks();
#endif
  void connect(bool initial);
};

#endif // BOARD_HAS_WIFI

#endif // WIFI_MANAGER_H
