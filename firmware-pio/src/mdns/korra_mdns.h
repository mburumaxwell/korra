#ifndef KORRA_MDNS_H
#define KORRA_MDNS_H

#include "korra_config.h"

#if BOARD_HAS_NETWORK

#include <Arduino.h>
#include <ArduinoMDNS.h>

/**
 * This class is a wrapper for the service discovery via mDNS/Bonjour.
 */
class KorraMdns
{
public:
  /**
   * Creates a new instance of the KorraMdns class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param udpClient The UDP client to use for the service discovery.
   */
  KorraMdns(UDP &udpClient);

  /**
   * Cleanup resources created and managed by the KorraMdns class.
   */
  ~KorraMdns();

  /**
   * Initiate mDNS
   *
   * @param ip IP address of the network interface (WiFi, Ethernet, etc).
   * @param hostname Hostname set on the network interface.
   * @param mac MAC address of the network interface.
   */
  void begin(IPAddress ip, const char *hostname, uint8_t *mac);

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

private:
  MDNS mdns;
};

#endif // BOARD_HAS_NETWORK

#endif // KORRA_MDNS_H
