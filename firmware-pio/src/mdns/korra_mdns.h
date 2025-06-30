#ifndef KORRA_MDNS_H
#define KORRA_MDNS_H

#include "korra_config.h"

#ifdef CONFIG_BOARD_HAS_INTERNET

#include <Arduino.h>
#include <ArduinoMDNS.h>

#include "internet/korra_network_shared.h"

/**
 * This class is a wrapper for the service discovery via mDNS/Bonjour.
 */
class KorraMdns {
public:
  /**
   * Creates a new instance of the KorraMdns class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param client The UDP client.
   */
  KorraMdns(UDP &client);

  /**
   * Cleanup resources created and managed by the KorraMdns class.
   */
  ~KorraMdns();

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain(const struct korra_network_props *props);

private:
  MDNS mdns;
  bool setup = false;
};

#endif // BOARD_HAS_INTERNET

#endif // KORRA_MDNS_H
