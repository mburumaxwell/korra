#ifndef KORRA_ACTUATOR_H
#define KORRA_ACTUATOR_H

#include "korra_config.h"
#include "sensors/korra_sensors.h"

#include <time.h>

struct korra_actuator_config {
  /** Whether or not the actuator is allowed */
  bool enabled;

  /**
   * Seconds for which the actuator should be active at a given time (range: 5-15)
   */
  uint16_t duration;

  /** Seconds to wait before the next actuation (range: 5-60) */
  uint16_t equilibrium_time;

  /**
   * The target minimum value.
   * @note This depends on the application.
   * @note For pot, it is moisture (percentage).
   * @note For keeper, it is temperature (Celsius).
   */
  float target_min;

  /**
   * The target maximum value.
   * @note This depends on the application.
   * @note For pot, it is moisture (percentage).
   * @note For keeper, it is temperature (Celsius).
   */
  float target_max;
};

struct korra_actuator_state {
  /** Number of times the actuator was activated */
  uint16_t count;

  /** Last time (UNIX since Epoch) the actuator was activated */
  time_t last_time;

  /** Total seconds the actuator was active */
  uint32_t total_duration;
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
   * Update the state for the actuator.
   * This should only be called for the first time to preserve state.
   */
  void set_state(const struct korra_actuator_state *value);

  /**
   * Registers callback that will be called each time the actuator state is updated.
   *
   * @param callback
   */
  inline void onStateUpdated(void (*callback)(const struct korra_actuator_state *value)) {
    state_updated_callback = callback;
  }

  /**
   * Returns the current state of the actuator.
   */
  inline const struct korra_actuator_state *state() { return &current_state; }

  /**
   * Returns the current configuration of the actuator.
   */
  inline const struct korra_actuator_config *config() { return &current_config; }

private:
  struct korra_actuator_config current_config = {0};
  struct korra_actuator_state current_state = {0};
  void (*state_updated_callback)(const struct korra_actuator_state *value) = NULL;

  unsigned long timepoint = 0;
  bool current_value_consumed = true; // prevents early actuation
  float current_value = 0;

private:
  void actuate(uint32_t duration_sec);
  void print_config();
  void print_state();
};

#endif // KORRA_ACTUATOR_H
