#ifndef KORRA_SENSORS_H
#define KORRA_SENSORS_H

#include <stdint.h>
#include "korra_config.h"

#ifdef APP_KIND_KEEPER
#include <DHTesp.h>
#endif // APP_KIND_KEEPER

#ifdef APP_KIND_POT
// #include <DFRobot_PH.h>
#endif // APP_KIND_POT

struct KorraSensorsData
{
#ifdef APP_KIND_KEEPER
  // measured in °C
  float temperature;

  // relative humidity (%)
  float humidity;
#endif // APP_KIND_KEEPER

#ifdef APP_KIND_POT
  // percentage (%) of water in a substance
  int32_t moisture;

  float ph;
#endif // APP_KIND_POT
};

/**
 * This class is a wrapper for the sensors logic.
 * It is where all the sensors related code is located.
 */
class KorraSensors
{
public:
  /**
   * Creates a new instance of the KorraSensors class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  KorraSensors();

  /**
   * Cleanup resources created and managed by the KorraSensors class.
   */
  ~KorraSensors();

  /**
   * Initializes the sensors.
   * This should be called once at the beginning of the program.
   * The required interrupts are also attached.
   */
  void begin();

  /**
   * Reads all sensor data and stores it in the provided KorraSensorsData structure.
   *
   * @param dest The destination structure to store the sensor data.
   */
  void read(KorraSensorsData *dest);

private:
#ifdef APP_KIND_KEEPER
  DHTesp dht;
#endif // APP_KIND_KEEPER

#ifdef APP_KIND_POT
  // DFRobot_PH phProbe; // pH probe
#endif // APP_KIND_POT

private:
#ifdef APP_KIND_POT
  float readPH();
  int32_t readMoisture();
#endif // APP_KIND_POT
};

#endif // KORRA_SENSORS_H
