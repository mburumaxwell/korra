#ifndef KORRA_CELLULAR_H
#define KORRA_CELLULAR_H

#include "korra_config.h"
#include "korra_network_shared.h"

#ifdef CONFIG_BOARD_HAS_CELLULAR

// #include <Cellular.h>
// #include <CellularUdp.h>

/**
 * This class is a wrapper for the Cellular logic.
 * It is where all the Cellular related code is located.
 */
class KorraCellular
{
public:
  /**
   * Creates a new instance of the KorraCellular class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  KorraCellular();

  /**
   * Cleanup resources created and managed by the KorraCellular class.
   */
  ~KorraCellular();

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
};

#endif // CONFIG_BOARD_HAS_CELLULAR

#endif // KORRA_CELLULAR_H
