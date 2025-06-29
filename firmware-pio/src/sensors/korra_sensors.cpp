#include <Arduino.h>
#include "korra_sensors.h"

KorraSensors::KorraSensors() {}

KorraSensors::~KorraSensors() {}

void KorraSensors::begin()
{
#ifdef CONFIG_APP_KIND_KEEPER
  // TODO: confirm model or switch to auto detect
  dht.setup(CONFIG_SENSORS_DHT21_PIN, DHTesp::DHT11);
  // dht.setup(CONFIG_SENSORS_DHT21_PIN, DHTesp::AUTO_DETECT);
#endif // CONFIG_APP_KIND_KEEPER
}

void KorraSensors::read(struct korra_sensors_data *dest)
{
#ifdef CONFIG_APP_KIND_KEEPER
  TempAndHumidity th = dht.getTempAndHumidity();
  dest->temperature = th.temperature;
  dest->humidity = th.humidity;
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  dest->moisture = readMoisture();
  dest->ph = readPH();
#endif // CONFIG_APP_KIND_POT
}

#ifdef CONFIG_APP_KIND_POT
float KorraSensors::readPH()
{
  // TODO: implement this once we have the soil sensor selected
  return -1;
}

int32_t KorraSensors::readMoisture()
{
  // TODO: Need to calibrate this
  int dry = 587;
  int wet = 84;
  int reading = analogRead(CONFIG_SENSORS_MOISTURE_PIN);
  return (int32_t)(100.0 * (dry - reading) / (dry - wet));
}
#endif // CONFIG_APP_KIND_POT
