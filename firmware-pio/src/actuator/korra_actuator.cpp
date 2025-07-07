#include <Arduino.h>

#include "korra_actuator.h"

#ifdef CONFIG_APP_KIND_KEEPER
#define TARGET_UNIT_STR "moisture (%%)"
#endif // CONFIG_APP_KIND_KEEPER
#ifdef CONFIG_APP_KIND_POT
#define TARGET_UNIT_STR "temperature (C)"
#endif // CONFIG_APP_KIND_POT

KorraActuator::KorraActuator() {
}

KorraActuator::~KorraActuator() {
  state_updated_callback = NULL;
}

void KorraActuator::begin() {
#ifdef CONFIG_APP_KIND_KEEPER
  pinMode(CONFIG_ACTUATORS_FAN_PIN, OUTPUT);
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  pinMode(CONFIG_ACTUATORS_PUMP_PIN, OUTPUT);
#endif // CONFIG_APP_KIND_POT

  timepoint = millis();
}

void KorraActuator::update(const struct korra_sensors_data *value) {
#ifdef CONFIG_APP_KIND_KEEPER
  current_value = value->temperature;
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  current_value = value->moisture.value;
#endif // CONFIG_APP_KIND_POT

  current_value_consumed = false;
}

void KorraActuator::maintain() {
  // if actuator is disabled return
  if (!current_config.enabled) return;

  // if we have not consumed current values, the current value is less/greater than the target value,
  // and the time since the last actuation is greater than the equilibrium time then actuate

  // for keeper, we target to not be above the current value (temperature)
  // for pot, we target to not be below the current value (moisture)
#ifdef CONFIG_APP_KIND_KEEPER
  bool should_actuate = (current_value > current_config.target);
#endif // CONFIG_APP_KIND_KEEPER
#ifdef CONFIG_APP_KIND_POT
  bool should_actuate = (current_value < current_config.target);
#endif // CONFIG_APP_KIND_POT

  if (!current_value_consumed && should_actuate) {
    // check if the time since the last actuation is greater than the equilibrium time
    const unsigned long elapsed_time = (millis() - timepoint) / 1000;
    if (elapsed_time > current_config.equilibrium_time) {
      // Actuating for a given duration
      const uint32_t duration = current_config.duration;
      Serial.printf("Actuating for %d sec, targeting %.2f " TARGET_UNIT_STR ", currently %.2f\n", duration,
                    current_config.target, current_value);
      actuate(duration);
      timepoint = millis(); // reset the timepoint (must be done after actuation)
      current_value_consumed = true;
      Serial.println(F("Actuation completed"));
    }
  }
}

void KorraActuator::set_config(const struct korra_actuator_config *value) {
  // copy the config, logic will update in the maintain() function
  memcpy(&current_config, value, sizeof(struct korra_actuator_config));
  Serial.println("Actuator Config set");
  print_config();
}

void KorraActuator::set_state(const struct korra_actuator_state *value) {
  memcpy(&current_state, value, sizeof(struct korra_actuator_state));
  Serial.println("Actuator State set");
  print_state();
}

void KorraActuator::actuate(uint32_t duration_sec) {
#ifdef CONFIG_APP_KIND_KEEPER

  digitalWrite(CONFIG_ACTUATORS_FAN_PIN, HIGH); // on
  delay(duration_sec * 1000);
  digitalWrite(CONFIG_ACTUATORS_FAN_PIN, LOW); // off

#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT

  digitalWrite(CONFIG_ACTUATORS_PUMP_PIN, HIGH); // on
  delay(duration_sec * 1000);
  digitalWrite(CONFIG_ACTUATORS_PUMP_PIN, LOW); // off

#endif // CONFIG_APP_KIND_POT

  // update the state
  current_state.count++;
  current_state.last_time = time(NULL);
  current_state.total_duration += duration_sec;
  if (state_updated_callback) {
    state_updated_callback(&current_state);
  }
}

void KorraActuator::print_config() {
  Serial.printf("Actuator Config: Enabled: %s\n", current_config.enabled ? "yes" : "no");
  Serial.printf("Actuator Config: Duration: %d seconds\n", current_config.duration);
  Serial.printf("Actuator Config: Equilibrium Time: %d seconds\n", current_config.equilibrium_time);
  Serial.printf("Actuator Config: Target: %.2f\n", current_config.target);
}

void KorraActuator::print_state() {
  Serial.printf("Actuator State: Count: %d\n", current_state.count);
  Serial.printf("Actuator State: Last Time: %d\n", current_state.last_time);
  Serial.printf("Actuator State: Total Duration: %d seconds\n", current_state.total_duration);
}
