# firmware

## 0.2.4

### Patch Changes

- b499cdb: Add support for ESP32-C6 DevKitC
- 061214c: Fix null terminator handling in generated device credential
- 965d4ce: Update zephyr to v4.2.0
- 6faef95: Remove support WPA Supplicant which is quite buggy

## 0.2.3

### Patch Changes

- 542e3ac: Restore sensor read interval back to 5 min

## 0.2.2

### Patch Changes

- 4347a1f: Change sensor read duration from 5min to 1 min so that the actuators can act better

## 0.2.1

### Patch Changes

- 5bdfe77: Label USERTrust certificate appropriately and add ECC version

## 0.2.0

### Minor Changes

- 41199b6: Add nRF7002DK board support
- 5f50af5: Add cloud provisioning, events, and net utils modules update configs and refactor cloud
- ecb2f59: Generate certificate for 3 years and use current time if available
- 81a8497: Add WiFi connection for internet access

### Patch Changes

- f7b2697: Added cloud support with Azure IoT DPS
- fab7ab2: Add time sync to pio firmware use UK SNTP servers in main firmware
- 585c1e5: Change telemetry interval from every minute to every 5 min
