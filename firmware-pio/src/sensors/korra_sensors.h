#ifndef KORRA_SENSORS_H
#define KORRA_SENSORS_H

#include <stdint.h>

#include "korra_config.h"

#ifdef CONFIG_APP_KIND_KEEPER
#include <DHTesp.h>
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
// #include <DFRobot_PH.h>
#endif // CONFIG_APP_KIND_POT

struct korra_analog_sensor_reading {
  /**
   * The raw value read from the analog sensor.
   * This is the raw value read from the analog pin, typically in millivolts.
   * It can be used to calculate the percentage or other derived values.
   */
  uint32_t millivolts;

  /**
   * The unit of the sensor reading.
   * This is a string that describes the unit of the value, such as "C" for Celsius,
   * "%" for percentage, etc.
   */
  float value;
};

struct korra_sensors_data {
#ifdef CONFIG_APP_KIND_KEEPER
  /** Measured in °C */
  float temperature;

  /** Relative humidity (%) */
  float humidity;
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  /** Percentage (%) of water in the soil */
  struct korra_analog_sensor_reading moisture;

  struct korra_analog_sensor_reading ph;
#endif // CONFIG_APP_KIND_POT
};

/**
 * This class is a wrapper for the sensors logic.
 * It is where all the sensors related code is located.
 */
class KorraSensors {
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
   * Initializes the sensors logic.
   * This should be called once at the beginning of the program.
   * The required interrupts are also attached.
   */
  void begin();

  /**
   * Reads all sensor data and stores it in the provided korra_sensors_data structure.
   *
   * @param dest The destination structure to store the sensor data.
   */
  void read(struct korra_sensors_data *dest);

private:
#ifdef CONFIG_APP_KIND_KEEPER
  DHTesp dht;
#endif // CONFIG_APP_KIND_KEEPER

#ifdef CONFIG_APP_KIND_POT
  // DFRobot_PH phProbe; // pH probe
#endif // CONFIG_APP_KIND_POT

private:
#ifdef CONFIG_APP_KIND_POT
  void read_ph(struct korra_analog_sensor_reading *reading);
  void read_moisture(struct korra_analog_sensor_reading *reading);
#endif // CONFIG_APP_KIND_POT
};

#endif // KORRA_SENSORS_H
