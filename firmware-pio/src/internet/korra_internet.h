#ifndef KORRA_INTERNET_H
#define KORRA_INTERNET_H

#include "korra_config.h"
#include "korra_network_shared.h"

#ifdef CONFIG_BOARD_HAS_INTERNET

#ifdef CONFIG_BOARD_HAS_CELLULAR
#include "korra_cellular.h"
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
#include "korra_ethernet.h"
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
#include "korra_wifi.h"
#endif // CONFIG_BOARD_HAS_WIFI

/**
 * This class is a wrapper for the Internet logic.
 * It is where all the Internet related code is located.
 */
class KorraInternet {
public:
  /**
   * Creates a new instance of the KorraInternet class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param prefs The preferences instance to use for storing the network credentials.
   */
  KorraInternet(Preferences &prefs)
#ifdef CONFIG_BOARD_HAS_CELLULAR
      : cellular(prefs)
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
      : ethernet()
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
      : wifi(prefs)
#endif // CONFIG_BOARD_HAS_WIFI
  {
  }

  /**
   * Cleanup resources created and managed by the KorraInternet class.
   */
  ~KorraInternet() {}

  /**
   * Initialize the Internet connection.
   *
   * @note This method should be called once during the initialization of the firmware.
   */
  inline void begin() {
#ifdef CONFIG_BOARD_HAS_CELLULAR
    cellular.begin();
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
    ethernet.begin();
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
    wifi.begin();
#endif // CONFIG_BOARD_HAS_WIFI
  }

  /**
   * Maintain the Internet connection.
   *
   * @note This method should be called periodically inside the main loop of the firmware.
   * @note It's safe to call this method in some interval (like 5ms).
   */
  inline void maintain() {
#ifdef CONFIG_BOARD_HAS_CELLULAR
    cellular.maintain();
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
    ethernet.maintain();
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
    wifi.maintain();
#endif // CONFIG_BOARD_HAS_WIFI
  }

#ifdef CONFIG_BOARD_HAS_WIFI
  /**
   * Save the WiFi credentials.
   *
   * @param credentials The credentials.
   */
  inline bool credentials_save_wifi(const struct wifi_credentials *credentials) {
    return wifi.credentials_save(credentials);
  }
#endif // CONFIG_BOARD_HAS_WIFI

  /**
   * Clear the internet credentials.
   */
  inline bool credentials_clear() {
#ifdef CONFIG_BOARD_HAS_WIFI
    return wifi.credentials_clear();
#endif // CONFIG_BOARD_HAS_WIFI
  }

  /**
   * Get the props of the currently connected network.
   */
  inline const struct korra_network_props *props() {
#ifdef CONFIG_BOARD_HAS_CELLULAR
    return cellular.props();
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
    return ethernet.props();
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
    return wifi.props();
#endif // CONFIG_BOARD_HAS_WIFI
  }

  /**
   * Whether connected to the network.
   */
  inline bool connected() {
#ifdef CONFIG_BOARD_HAS_CELLULAR
    return cellular.connected();
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
    return ethernet.connected();
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
    return wifi.connected();
#endif // CONFIG_BOARD_HAS_WIFI
  }

private:
#ifdef CONFIG_BOARD_HAS_CELLULAR
  KorraCellular cellular;
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
  KorraEthernet ethernet;
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
  KorraWiFi wifi;
#endif // CONFIG_BOARD_HAS_WIFI
};

#endif // CONFIG_BOARD_HAS_INTERNET

#endif // KORRA_INTERNET_H
