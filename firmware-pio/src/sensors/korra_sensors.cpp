#include <Arduino.h>

#include "korra_sensors.h"

KorraSensors::KorraSensors() {
}

KorraSensors::~KorraSensors() {
}

void KorraSensors::begin() {
#ifdef CONFIG_APP_KIND_KEEPER
  dht.setup(CONFIG_SENSORS_DHT21_PIN, DHTesp::DHT11);
#endif // CONFIG_APP_KIND_KEEPER
}

void KorraSensors::read(struct korra_sensors_data *dest) {
#ifdef CONFIG_APP_KIND_KEEPER
  TempAndHumidity th = dht.getTempAndHumidity();
  dest->temperature = th.temperature;
  dest->humidity = th.humidity;
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  dest->moisture = read_moisture();
  dest->ph = read_ph();
#endif // CONFIG_APP_KIND_POT
}

#ifdef CONFIG_APP_KIND_POT
float KorraSensors::read_ph() {
  // TODO: implement this once we have the soil sensor selected
  return -1;
}

uint8_t KorraSensors::read_moisture() {
  // TODO: May need to recalibrate this from time to time or sensor to sensor, if too much move it to device twin
  static const uint32_t dry = 2565;
  static const uint32_t wet = 1263;
  uint32_t reading = analogReadMilliVolts(CONFIG_SENSORS_MOISTURE_PIN);
  uint8_t value = (uint8_t)(100.0 * (dry - reading) / (dry - wet));
  return (uint8_t)std::ranges::clamp((int)value, 0, 100);
}
#endif // CONFIG_APP_KIND_POT
