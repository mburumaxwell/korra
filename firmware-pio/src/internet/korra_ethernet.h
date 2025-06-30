#ifndef KORRA_ETHERNET_H
#define KORRA_ETHERNET_H

#include "korra_config.h"
#include "korra_network_shared.h"

#ifdef CONFIG_BOARD_HAS_ETHERNET

// #include <Ethernet.h>
// #include <EthernetUdp.h>

/**
 * This class is a wrapper for the Ethernet logic.
 * It is where all the Ethernet related code is located.
 */
class KorraEthernet {
public:
  /**
   * Creates a new instance of the KorraEthernet class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  KorraEthernet();

  /**
   * Cleanup resources created and managed by the KorraEthernet class.
   */
  ~KorraEthernet();

  /**
   * Connect to ethernet.
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

  /**
   * Whether connected to the network.
   */
  bool connected();

private:
  uint8_t _status;
  korra_network_props net_props;
};

#endif // CONFIG_BOARD_HAS_ETHERNET

#endif // KORRA_ETHERNET_H
