#ifndef KORRA_ACTUATOR_H
#define KORRA_ACTUATOR_H

#include "korra_config.h"
#include "sensors/korra_sensors.h"

#ifdef CONFIG_APP_KIND_KEEPER
#include <ESP32Servo.h>
// #include <Servo.h>
#endif // CONFIG_APP_KIND_KEEPER

struct korra_actuator_config {
  /** Whether or not the actuator is allowed */
  bool enabled;

  /**
   * Seconds for which the actuator should be active at a given time (range: 5-60)
   */
  uint16_t duration;

  /**
   * The target value.
   * @note This depends on the application.
   * @note For pot, it is moisture (percentage).
   * @note For keeper, it is temperature (Celsius).
   */
  float target;

  /** Seconds to wait before the next actuation (range: 5-60) */
  uint16_t equilibrium_time; //
};

struct korra_actuator_state {
  /** Number of times the actuator was activated */
  uint16_t count;

  /** Last time the actuator was activated */
  time_t last_time;

  /** Total time the actuator was active */
  uint32_t total_duration;
};

class KorraActuator {
public:
  KorraActuator();
  ~KorraActuator();

  void begin();
  void maintain();
  void update(const struct korra_sensors_data *value);
  void set_config(const struct korra_actuator_config *value);

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
  korra_actuator_config current_config = {0};
  korra_actuator_state current_state = {0};
  void (*state_updated_callback)(const struct korra_actuator_state *value) = NULL;

  unsigned long timepoint = 0;
  bool current_value_consumed = false;
  float current_value = 0;

#ifdef CONFIG_APP_KIND_KEEPER
  Servo fan;
#endif // CONFIG_APP_KIND_KEEPER

private:
  void actuate();
};

#endif // KORRA_ACTUATOR_H
