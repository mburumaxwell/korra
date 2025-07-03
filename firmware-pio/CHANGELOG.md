# firmware-pio

## 0.2.0

### Minor Changes

- 751d5de: Enhance sensor data structure by adding units and including timestamp
- d79121c: Add credentials support in pio firmware
- b2bb7a8: Target board of lower flash memory
- bddb17e: Complete wrapper for internet logic
- f7b2697: Added cloud support with Azure IoT DPS
- f19f18d: Add OTA update functionality
- 74ff010: Change WiFi credentials storage to use JSON for simplicity
- 2c05f39: Added platformio-based firmware. This is meant to catapult the project since so much already works with Arduino/ESP32
- 675e91c: Added device twin logic to house firmware version and update info
- c6c6774: Add actuator functionality which syncs with device twin
- 13b6556: Complete cloud telemetry (D2C)
- 365f035: Add Wi-Fi credential management hence avoid hard coded ones

### Patch Changes

- 121fe8b: Add command to clear provisioning info
- 31c4b0c: Set correct sensor pins for DHT and moisture readings
- cc7b231: Move WiFi to event based to avoid blocking processing of the shell commands
- fab7ab2: Add time sync to pio firmware use UK SNTP servers in main firmware
- 585c1e5: Change telemetry interval from every minute to every 5 min
- f0e03a0: Setting wifi credentials should clear the existing key
- 5230f06: Add command to clear device credential
- 06fd42f: Add 'prefs-clear' command to the shell for clearing all preferences
