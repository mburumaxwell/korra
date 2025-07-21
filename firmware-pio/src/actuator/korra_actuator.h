#ifndef KORRA_ACTUATOR_H
#define KORRA_ACTUATOR_H

#include "korra_config.h"
#include "sensors/korra_sensors.h"

#include <time.h>

struct korra_actuator_config {
  /** Whether or not the actuator is allowed */
  bool enabled;

  /**
   * Seconds for which the actuator should be active at a given time (range: 5-60)
   */
  uint16_t duration;

  /** Seconds to wait before the next actuation (range: 3-60) */
  uint16_t equilibrium_time;

  /**
   * The target value.
   * @note This depends on the application.
   * @note For keeper, it is temperature (Celsius); we target to not be above this.
   * @note For pot, it is moisture (percentage); we target to not be below this.
   */
  float target;
};

struct korra_actuation {
  /** Last time (UNIX since Epoch) the actuator was activated */
  time_t timestamp;

  /** Total seconds the actuator was active */
  uint16_t duration;
};

class KorraActuator {
public:
  /**
   * Creates a new instance of the KorraActuator class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  KorraActuator();

  /**
   * Cleanup resources created and managed by the KorraActuator class.
   */
  ~KorraActuator();

  /**
   * Initializes the cloud provisioning logic.
   * This should be called once at the beginning of the program.
   */
  void begin();

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

  /**
   * Update the sensor values relied on by the actuator.
   *
   * @param value The sensor values collected.
   */
  void update(const struct korra_sensors_data *value);

  /**
   * Update the config used by the actuator.
   */
  void set_config(const struct korra_actuator_config *value);

  /**
   * Registers callback that will be called each time actuation happens.
   *
   * @param callback
   */
  inline void onActuated(void (*callback)(const struct korra_actuation *value)) {
    actuated_callback = callback;
  }

  /**
   * Returns the current configuration of the actuator.
   */
  inline const struct korra_actuator_config *config() { return &current_config; }

private:
  struct korra_actuator_config current_config = {0};
  void (*actuated_callback)(const struct korra_actuation *value) = NULL;

  unsigned long timepoint = 0;
  bool current_value_consumed = true; // prevents early actuation
  float current_value = 0;

private:
  void actuate(uint16_t duration_sec);
  void print_config();
};

#endif // KORRA_ACTUATOR_H
