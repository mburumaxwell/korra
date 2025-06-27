#ifndef KORRA_TIME_H
#define KORRA_TIME_H

#include "korra_config.h"

#if BOARD_HAS_NETWORK

#include <Arduino.h>
#include <NTPClient.h>
#include <time.h>

/**
 * This class is a wrapper for the time sync.
 */
class KorraTime
{
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
   * This should be called once at the beginning of the program.
   */
  void begin();

  /**
   * Sync time once.
   * Call this if you need to ensure sync early such as in main.
   */
  void sync();

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

private:
  NTPClient client;
};

#endif // BOARD_HAS_NETWORK

#endif // KORRA_TIME_H
