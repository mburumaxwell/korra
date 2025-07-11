/******************************************************************
  DHT Temperature & Humidity Sensor library for Arduino & ESP32.

  Features:
  - Support for DHT11 and DHT22/AM2302/RHT03
  - Auto detect sensor model
  - Very low memory footprint
  - Very small code

  https://github.com/beegee-tokyo/arduino-DHTesp

  Written by Mark Ruys, mark@paracas.nl.
  Updated to work with ESP32 by Bernd Giesecke, bernd@giesecke.tk

  GNU General Public License, check LICENSE for more information.
  All text above must be included in any redistribution.

  Datasheets:
  - http://www.micro4you.com/files/sensor/DHT11.pdf
  - http://www.adafruit.com/datasheets/DHT22.pdf
  - http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Weather/RHT03.pdf
  - http://meteobox.tk/files/AM2302.pdf

  Changelog:
    see README.md
******************************************************************/

#include "DHTesp.h"

void DHTesp::setup(uint8_t pin, DHT_MODEL_t model) {
  DHTesp::pin = pin;
  DHTesp::model = model;
  DHTesp::resetTimer(); // Make sure we do read the sensor in the next readSensor()

  if (model == AUTO_DETECT) {
    DHTesp::model = DHT22;
    readSensor();
    if (error == ERROR_TIMEOUT) {
      DHTesp::model = DHT11;
      // Warning: in case we auto detect a DHT11, you should wait at least 1000 msec
      // before your first read request. Otherwise you will get a time out error.
    }
  }

  // Set default comfort profile.

  // In computing these constants the following reference was used
  // http://epb.apogee.net/res/refcomf.asp
  // It was simplified as 4 straight lines and added very little skew on
  // the vertical lines (+0.1 on x for C,D)
  // The for points used are(from top left, clock wise)
  // A(30%, 30*C) B(70%, 26.2*C) C(70.1%, 20.55*C) D(30.1%, 22.22*C)
  // On the X axis we have the rel humidity in % and on the Y axis the temperature in *C

  // Too hot line AB
  m_comfort.m_tooHot_m = -0.095;
  m_comfort.m_tooHot_b = 32.85;
  // Too humid line BC
  m_comfort.m_tooHumid_m = -56.5;
  m_comfort.m_tooHumid_b = 3981.2;
  // Too cold line DC
  m_comfort.m_tooCold_m = -0.04175;
  m_comfort.m_tooHCold_b = 23.476675;
  // Too dry line AD
  m_comfort.m_tooDry_m = -77.8;
  m_comfort.m_tooDry_b = 2364;
}

void DHTesp::resetTimer() {
  DHTesp::lastReadTime = millis() - 3000;
}

float DHTesp::getHumidity() {
  readSensor();
  if (error == ERROR_TIMEOUT) { // Try a second time to read
    readSensor();
  }
  return humidity;
}

float DHTesp::getTemperature() {
  readSensor();
  if (error == ERROR_TIMEOUT) { // Try a second time to read
    readSensor();
  }
  return temperature;
}

TempAndHumidity DHTesp::getTempAndHumidity() {
  readSensor();
  if (error == ERROR_TIMEOUT) { // Try a second time to read
    readSensor();
  }
  values.temperature = temperature;
  values.humidity = humidity;
  return values;
}

#ifndef OPTIMIZE_SRAM_SIZE

const char *DHTesp::getStatusString() {
  switch (error) {
  case DHTesp::ERROR_TIMEOUT:
    return "TIMEOUT";

  case DHTesp::ERROR_CHECKSUM:
    return "CHECKSUM";

  default:
    return "OK";
  }
}

#else

// At the expense of 26 bytes of extra PROGMEM, we save 11 bytes of
// SRAM by using the following method:

prog_char P_OK[] PROGMEM = "OK";
prog_char P_TIMEOUT[] PROGMEM = "TIMEOUT";
prog_char P_CHECKSUM[] PROGMEM = "CHECKSUM";

const char *DHTesp::getStatusString() {
  prog_char *c;
  switch (error) {
  case DHTesp::ERROR_CHECKSUM:
    c = P_CHECKSUM;
    break;

  case DHTesp::ERROR_TIMEOUT:
    c = P_TIMEOUT;
    break;

  default:
    c = P_OK;
    break;
  }

  static char buffer[9];
  strcpy_P(buffer, c);

  return buffer;
}

#endif

void DHTesp::readSensor() {
  // Make sure we don't poll the sensor too often
  // - Max sample rate DHT11 is 1 Hz   (duty cycle 1000 ms)
  // - Max sample rate DHT22 is 0.5 Hz (duty cycle 2000 ms)
  unsigned long startTime = millis();
  if ((unsigned long)(startTime - lastReadTime) < (model == DHT11 ? 999L : 1999L)) {
    return;
  }
  lastReadTime = startTime;

  temperature = NAN;
  humidity = NAN;

  uint16_t rawHumidity = 0;
  uint16_t rawTemperature = 0;
  uint16_t data = 0;

  // Request sample
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW); // Send start signal
  if (model == DHT11) {
    delay(18);
  } else {
    // This will fail for a DHT11 - that's how we can detect such a device
    delay(2);
  }

  digitalWrite(pin, HIGH); // Switch bus to receive data
  pinMode(pin, INPUT);

  // We're going to read 83 edges:
  // - First a FALLING, RISING, and FALLING edge for the start bit
  // - Then 40 bits: RISING and then a FALLING edge per bit
  // To keep our code simple, we accept any HIGH or LOW reading if it's max 85 usecs long

#ifdef ESP32
  // ESP32 is a multi core / multi processing chip
  // It is necessary to disable task switches during the readings
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&mux);
#else
  //   cli();
  noInterrupts();
#endif
  for (int8_t i = -3; i < 2 * 40; i++) {
    byte age;
    startTime = micros();

    do {
      age = (unsigned long)(micros() - startTime);
      if (age > 90) {
        error = ERROR_TIMEOUT;
#ifdef ESP32
        portEXIT_CRITICAL(&mux);
#else
        // sei();
        interrupts();
#endif
        return;
      }
    } while (digitalRead(pin) == (i & 1) ? HIGH : LOW);

    if (i >= 0 && (i & 1)) {
      // Now we are being fed our 40 bits
      data <<= 1;

      // A zero max 30 usecs, a one at least 68 usecs.
      if (age > 30) {
        data |= 1; // we got a one
      }
    }

    switch (i) {
    case 31:
      rawHumidity = data;
      break;
    case 63:
      rawTemperature = data;
      data = 0;
      break;
    }
  }

#ifdef ESP32
  portEXIT_CRITICAL(&mux);
#else
  //   sei();
  interrupts();
#endif

  // Verify checksum

  if (static_cast<byte>(((byte)rawHumidity) + (rawHumidity >> 8) + ((byte)rawTemperature) + (rawTemperature >> 8)) !=
      data) {
    error = ERROR_CHECKSUM;
    return;
  }

  // Store readings

  if (model == DHT11) {
    humidity = (rawHumidity >> 8) + ((rawHumidity & 0x00FF) * 0.1);
    temperature = (rawTemperature >> 8) + ((rawTemperature & 0x007F) * 0.1);
    if (rawTemperature & 0x0080) {
      temperature = -temperature;
    }
  } else {
    humidity = rawHumidity * 0.1;

    if (rawTemperature & 0x8000) {
      rawTemperature = -static_cast<int16_t>(rawTemperature & 0x7FFF);
    }
    temperature = static_cast<int16_t>(rawTemperature) * 0.1;
  }

  error = ERROR_NONE;
}

// boolean isFahrenheit: True == Fahrenheit; False == Celsius
float DHTesp::computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit) {
  // Using both Rothfusz and Steadman's equations
  // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
  float hi;

  if (!isFahrenheit) {
    temperature = toFahrenheit(temperature);
  }

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79) {
    hi = -42.379 + 2.04901523 * temperature + 10.14333127 * percentHumidity +
         -0.22475541 * temperature * percentHumidity + -0.00683783 * pow(temperature, 2) +
         -0.05481717 * pow(percentHumidity, 2) + 0.00122874 * pow(temperature, 2) * percentHumidity +
         0.00085282 * temperature * pow(percentHumidity, 2) +
         -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if ((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if ((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  return isFahrenheit ? hi : toCelsius(hi);
}

// boolean isFahrenheit: True == Fahrenheit; False == Celsius
float DHTesp::computeDewPoint(float temperature, float percentHumidity, bool isFahrenheit) {
  // reference: http://wahiduddin.net/calc/density_algorithms.htm
  if (isFahrenheit) {
    temperature = toCelsius(temperature);
  }
  double calc_temp = 373.15 / (273.15 + static_cast<double>(temperature));
  double calc_sum = -7.90298 * (calc_temp - 1);
  calc_sum += 5.02808 * log10(calc_temp);
  calc_sum += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / calc_temp))) - 1);
  calc_sum += 8.1328e-3 * (pow(10, (-3.49149 * (calc_temp - 1))) - 1);
  calc_sum += log10(1013.246);
  double calc_value = pow(10, calc_sum - 3) * static_cast<double>(percentHumidity);
  double calc_dew_temp = log(calc_value / 0.61078); // temp var
  calc_dew_temp = (241.88 * calc_dew_temp) / (17.558 - calc_dew_temp);
  return isFahrenheit ? toFahrenheit(calc_dew_temp) : calc_dew_temp;
}

// boolean isFahrenheit: True == Fahrenheit; False == Celsius
byte DHTesp::computePerception(float temperature, float percentHumidity, bool isFahrenheit) {
  // Computing human perception from dew point
  // reference: https://en.wikipedia.org/wiki/Dew_point ==> Relationship to human comfort
  // reference: Horstmeyer, Steve (2006-08-15). "Relative Humidity....Relative to What? The Dew Point Temperature...a
  // better approach". Steve Horstmeyer, Meteorologist, WKRC TV, Cincinnati, Ohio, USA. Retrieved 2009-08-20. Using
  // table Return value Dew point    Human perception[6]
  //    7         Over 26 °C   Severely high, even deadly for asthma related illnesses
  //    6         24–26 °C     Extremely uncomfortable, oppressive
  //    5         21–24 °C     Very humid, quite uncomfortable
  //    4         18–21 °C     Somewhat uncomfortable for most people at upper edge
  //    3         16–18 °C     OK for most, but all perceive the humidity at upper edge
  //    2         13–16 °C     Comfortable
  //    1         10–12 °C     Very comfortable
  //    0         Under 10 °C  A bit dry for some

  if (isFahrenheit) {
    temperature = toCelsius(temperature);
  }
  float dewPoint = computeDewPoint(temperature, percentHumidity);

  if (dewPoint < 10.0f) {
    return Perception_Dry;
  } else if (dewPoint < 13.0f) {
    return Perception_VeryComfy;
  } else if (dewPoint < 16.0f) {
    return Perception_Comfy;
  } else if (dewPoint < 18.0f) {
    return Perception_Ok;
  } else if (dewPoint < 21.0f) {
    return Perception_UnComfy;
  } else if (dewPoint < 24.0f) {
    return Perception_QuiteUnComfy;
  } else if (dewPoint < 26.0f) {
    return Perception_VeryUnComfy;
  }
  // else dew >= 26.0
  return Perception_SevereUncomfy;
}

// boolean isFahrenheit: True == Fahrenheit; False == Celsius
float DHTesp::getComfortRatio(ComfortState &destComfortStatus, float temperature, float percentHumidity,
                              bool isFahrenheit) {
  float ratio = 100; // 100%
  float distance = 0;
  float kTempFactor = 3;    // take into account the slope of the lines
  float kHumidFactor = 0.1; // take into account the slope of the lines
  uint8_t tempComfort = 0;

  if (isFahrenheit) {
    temperature = toCelsius(temperature);
  }

  destComfortStatus = Comfort_OK;

  distance = m_comfort.distanceTooHot(temperature, percentHumidity);
  if (distance > 0) {
    // update the comfort descriptor
    tempComfort += static_cast<uint8_t>(Comfort_TooHot);
    // decrease the comfot ratio taking the distance into account
    ratio -= distance * kTempFactor;
  }

  distance = m_comfort.distanceTooHumid(temperature, percentHumidity);
  if (distance > 0) {
    // update the comfort descriptor
    tempComfort += static_cast<uint8_t>(Comfort_TooHumid);
    // decrease the comfot ratio taking the distance into account
    ratio -= distance * kHumidFactor;
  }

  distance = m_comfort.distanceTooCold(temperature, percentHumidity);
  if (distance > 0) {
    // update the comfort descriptor
    tempComfort += static_cast<uint8_t>(Comfort_TooCold);
    // decrease the comfot ratio taking the distance into account
    ratio -= distance * kTempFactor;
  }

  distance = m_comfort.distanceTooDry(temperature, percentHumidity);
  if (distance > 0) {
    // update the comfort descriptor
    tempComfort += static_cast<uint8_t>(Comfort_TooDry);
    // decrease the comfot ratio taking the distance into account
    ratio -= distance * kHumidFactor;
  }

  destComfortStatus = static_cast<ComfortState>(tempComfort);

  if (ratio < 0) ratio = 0;

  return ratio;
}

float DHTesp::computeAbsoluteHumidity(float temperature, float percentHumidity, bool isFahrenheit) {
  // Calculate the absolute humidity in g/m³
  // https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
  if (isFahrenheit) {
    temperature = toCelsius(temperature);
  }

  float absHumidity;
  float absTemperature;
  absTemperature = temperature + 273.15;

  absHumidity = 6.112;
  absHumidity *= exp((17.67 * temperature) / (243.5 + temperature));
  absHumidity *= percentHumidity;
  absHumidity *= 2.1674;
  absHumidity /= absTemperature;

  return absHumidity;
}
