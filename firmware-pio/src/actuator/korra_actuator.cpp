#include <Arduino.h>

#include "korra_actuator.h"

KorraActuator::KorraActuator() {
}

KorraActuator::~KorraActuator() {
  state_updated_callback = nullptr;
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
  current_value = value->moisture;
#endif // CONFIG_APP_KIND_POT

  current_value_consumed = false;
}

void KorraActuator::maintain() {
  // if actuator is disabled return
  if (!current_config.enabled) return;

  // if we have not consumed current values, the current value is less than the target value,
  // and the time since the last actuation is greater than the equilibrium time then actuate

  // check if the current value is less than the target value
  if (!current_value_consumed && current_value < current_config.target) {
    // check if the time since the last actuation is greater than the equilibrium time
    const auto elapsed_time = (millis() - timepoint) / 1000;
    if (elapsed_time > current_config.equilibrium_time) {
      // Actuating for a given duration
      const uint32_t duration = current_config.duration * 1000;
#ifdef CONFIG_APP_KIND_KEEPER
      Serial.printf("Actuating for %d ms, targeting %d% moisture, currently %d%\n", duration, current_config.target,
                    current_value);
#endif // CONFIG_APP_KIND_KEEPER
#ifdef CONFIG_APP_KIND_POT
      Serial.printf("Actuating for %d ms, targeting %d temperature, currently %d\n", duration, current_config.target,
                    current_value);
#endif // CONFIG_APP_KIND_POT
      actuate(current_config.duration * 1000);
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

void KorraActuator::actuate(uint32_t duration_ms) {
#ifdef CONFIG_APP_KIND_KEEPER

  digitalWrite(CONFIG_ACTUATORS_FAN_PIN, HIGH); // on
  delay(duration_ms);
  digitalWrite(CONFIG_ACTUATORS_FAN_PIN, LOW); // off

#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT

  digitalWrite(CONFIG_ACTUATORS_PUMP_PIN, HIGH); // on
  delay(duration_ms);
  digitalWrite(CONFIG_ACTUATORS_PUMP_PIN, LOW); // off

#endif // CONFIG_APP_KIND_POT

  // update the state
  current_state.count++;
  current_state.last_time = time(NULL);
  current_state.total_duration += duration_ms;
  if (state_updated_callback) {
    state_updated_callback(&current_state);
  }
}

void KorraActuator::print_config() {
  Serial.printf("Actuator Config: Enabled: %s\n", current_config.enabled ? "yes" : "no");
  Serial.printf("Actuator Config: Duration: %d seconds\n", current_config.duration);
  Serial.printf("Actuator Config: Equilibrium Time: %d seconds\n", current_config.equilibrium_time);
  Serial.printf("Actuator Config: Target: %f\n", current_config.target);
}
