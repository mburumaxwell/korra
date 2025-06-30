#include "korra_actuator.h"

// good default values for the actuator instead of having them as build flags
static const struct korra_actuator_config default_config = {
    .enabled = false,
    .duration = 5,

#ifdef CONFIG_APP_KIND_KEEPER
    .target = 34.0f,
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
    .target = 50.0f,
#endif // CONFIG_APP_KIND_POT

    .equilibrium_time = 3,
};

KorraActuator::KorraActuator() {
  memcpy(&this->current_config, &default_config, sizeof(struct korra_actuator_config));
}

KorraActuator::~KorraActuator() {
#ifdef CONFIG_APP_KIND_KEEPER
  fan.detach();
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  pump.detach();
#endif // CONFIG_APP_KIND_POT

  state_updated_callback = nullptr;
}

void KorraActuator::begin() {
#ifdef CONFIG_APP_KIND_KEEPER
  fan.attach(CONFIG_ACTUATORS_FAN_PIN);
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  pump.attach(CONFIG_ACTUATORS_PUMP_PIN);
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
      actuate();
      timepoint = millis(); // reset the timepoint (must be done after actuation)
      current_value_consumed = true;
    }
  }
}

void KorraActuator::set_config(const struct korra_actuator_config *value) {
  // copy the config, logic will update in the maintain() function
  memcpy(&current_config, value, sizeof(struct korra_actuator_config));
}

void KorraActuator::actuate() {
  // Actuating for a given duration
  // 0 is full speed forward
  // 90 is stopped
  // 180 is full speed reverse
  const uint32_t duration = current_config.duration * 1000;
  Serial.printf("Actuating for %d ms\n", duration);

#ifdef CONFIG_APP_KIND_KEEPER

  fan.write(0); // full speed forward
  delay(duration);
  fan.write(90); // stop

#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT

  pump.write(0); // full speed forward
  delay(duration);
  pump.write(90); // stop

#endif // CONFIG_APP_KIND_POT

  // update the state
  current_state.count++;
  current_state.last_time = time(NULL);
  current_state.total_duration += duration;
  if (state_updated_callback) {
    state_updated_callback(&current_state);
  }
  Serial.println(F("Actuation completed"));
}
