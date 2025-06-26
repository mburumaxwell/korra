#include <Arduino.h>
#include "korra_sensors.h"

KorraSensors::KorraSensors()
{
}

KorraSensors::~KorraSensors()
{
}

void KorraSensors::begin()
{
#ifdef APP_KIND_KEEPER
  // TODO: set correct pin
  // dht.setup(SENSORS_DHT22_PIN, DHTesp::DHT22);
#endif // APP_KIND_KEEPER
}

void KorraSensors::read(KorraSensorsData *dest)
{
#if APP_KIND_KEEPER
  TempAndHumidity th = dht.getTempAndHumidity();
  dest->temperature = th.temperature;
  dest->humidity = th.humidity;
#endif // APP_KIND_KEEPER

#if APP_KIND_POT
  dest->moisture = readMoisture();
  dest->ph = readPH();
#endif // APP_KIND_POT
}

#if APP_KIND_POT
float KorraSensors::readPH()
{
  return -1;
}

int32_t KorraSensors::readMoisture()
{
  // TODO; restore this after pin assignment

  // // Need to calibrate this
  // int dry = 587;
  // int wet = 84;
  // int reading = analogRead(SENSORS_MOISTURE_PIN);
  // return (int32_t)(100.0 * (dry - reading) / (dry - wet));
  return -1;
}
#endif // APP_KIND_KEEPER