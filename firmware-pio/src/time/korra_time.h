#ifndef KORRA_TIME_H
#define KORRA_TIME_H

#include "korra_config.h"

#ifdef CONFIG_BOARD_HAS_INTERNET

#include <Arduino.h>
#include <NTPClient.h>
#include <time.h>

/**
 * This class is a wrapper for the time sync.
 */
class KorraTime {
public:
  /**
   * Creates a new instance of the KorraTime class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param client The UDP client to use.
   */
  KorraTime(UDP &client);

  /**
   * Cleanup resources created and managed by the KorraTime class.
   */
  ~KorraTime();

  /**
   * Initializes the time logic.
   * This should be called once at the begining of the program.
   *
   * @param update_interval_sec The interval in seconds to update the time.
   */
  void begin(uint32_t update_interval_sec = 60);

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

private:
  NTPClient client;
};

#endif // BOARD_HAS_INTERNET

#endif // KORRA_TIME_H
