#include <Arduino.h>

#include "korra_sensors.h"

KorraSensors::KorraSensors() {
}

KorraSensors::~KorraSensors() {
}

void KorraSensors::begin() {
#ifdef CONFIG_APP_KIND_KEEPER
  dht.setup(CONFIG_SENSORS_DHT_PIN, DHTesp::DHT11);
#endif // CONFIG_APP_KIND_KEEPER
}

void KorraSensors::read(struct korra_sensors_data *dest) {
#ifdef CONFIG_APP_KIND_KEEPER
  TempAndHumidity th = dht.getTempAndHumidity();
  dest->temperature = th.temperature;
  dest->humidity = th.humidity;
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  read_moisture(&(dest->moisture));
  read_ph(&(dest->ph));
#endif // CONFIG_APP_KIND_POT
}

#ifdef CONFIG_APP_KIND_POT
void KorraSensors::read_ph(struct korra_analog_sensor_reading *reading) {
  // TODO: implement this once we have the soil sensor selected
  reading->millivolts = -1;
  reading->value = -1;
}

void KorraSensors::read_moisture(struct korra_analog_sensor_reading *reading, const uint8_t samples, const uint8_t interval_ms) {
  // TODO: May need to recalibrate this from time to time or sensor to sensor, if too much move it to device twin
  // There is an inverse ratio between the sensor output value and soil moisture.
  // https://wiki.dfrobot.com/Capacitive_Soil_Moisture_Sensor_SKU_SEN0193
  static const uint32_t dry = 2565;
  static const uint32_t wet = 1263;

  uint32_t values[samples];
  for (int i = 0; i < samples; i++) {
    values[i] = analogReadMilliVolts(CONFIG_SENSORS_MOISTURE_PIN);
    delay(interval_ms);
  }
  uint64_t sum = 0;
  for (int i = 0; i < samples; i++) sum += values[i];

  uint32_t millivolts = reading->millivolts = sum / samples;
  float value = reading->value = CLAMP((100.0 * (dry - millivolts) / (dry - wet)), 0, 100);
}
#endif // CONFIG_APP_KIND_POT
