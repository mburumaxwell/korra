#ifndef KORRA_WIFI_H
#define KORRA_WIFI_H

#include "korra_config.h"
#include "korra_network_shared.h"

#ifdef CONFIG_BOARD_HAS_WIFI

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

struct wifi_credentials
{
  /** The SSID of the network. */
  char ssid[32 + 1];

  /** The passphrase of the network. */
  char passphrase[50 + 1];

  /** The EAP identity of the network. */
  char eap_identity[50 + 1];

  /** The EAP username of the network. */
  char eap_username[50 + 1];

  /** The EAP password of the network. */
  char eap_password[50 + 1];
};

/**
 * This class is a wrapper for the WiFi logic.
 * It is where all the WiFi related code is located.
 */
class KorraWiFi
{
public:
  /**
   * Creates a new instance of the KorraWiFi class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param prefs The preferences instance to use for storing the WiFi credentials.
   */
  KorraWiFi(Preferences &prefs);

  /**
   * Cleanup resources created and managed by the KorraWiFi class.
   */
  ~KorraWiFi();

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
   * Save the credentials to the preferences.
   *
   * @param credentials The credentials.
   */
  bool credentials_save(const struct wifi_credentials *credentials);

  /**
   * Clear the credentials from the preferences.
   */
  bool credentials_clear();

  /**
   * Get the props of the network.
   */
  inline const struct korra_network_props *props() { return &net_props; }

  /**
   * Whether connected to the network.
   */
  inline bool connected() { return WiFi.isConnected(); }

  /**
   * Returns existing instance (singleton) of the KorraWiFi class.
   * It may be a null pointer if the KorraWiFi object was never constructed or it was destroyed.
   */
  inline static KorraWiFi *instance() { return _instance; }

  /**
   * Please do not call this method from outside the `KorraWiFi` class
   */
  void on_wifi_event(WiFiEvent_t event, WiFiEventInfo_t info);

  /// Living instance of the KorraWiFi class. It can be nullptr.
  static KorraWiFi *_instance;

private:
  Preferences &prefs;
  bool connection_requested = 0;
  struct wifi_credentials credentials = {0};
  struct korra_network_props net_props = {0};
  bool logged_missing_creds;

private:
#ifdef CONFIG_WIFI_SCAN_NETWORKS
  void listNetworks();
#endif // CONFIG_WIFI_SCAN_NETWORKS

  void connect();
  void credentials_load();
  void credentials_print();
};

#endif // BOARD_HAS_WIFI

#endif // KORRA_WIFI_H
