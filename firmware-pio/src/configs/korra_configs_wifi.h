#ifndef KORRA_CONFIGS_WIFI_H
#define KORRA_CONFIGS_WIFI_H

#ifdef ARDUINO_ARCH_ESP32
#define BOARD_HAS_WIFI 1
#else
#define BOARD_HAS_WIFI 0
#endif

#if BOARD_HAS_WIFI
// The amount of time to wait for a WiFi connection before rebooting
#define CONFIG_WIFI_CONNECTION_REBOOT_TIMEOUT_MILLIS 15000 // 15 seconds

#if (defined(WIFI_ENTERPRISE_USERNAME) && !defined(WIFI_ENTERPRISE_PASSWORD)) || \
    (!defined(WIFI_ENTERPRISE_USERNAME) && defined(WIFI_ENTERPRISE_PASSWORD))
#error "Enterprise WIFI (802.1x) requires both username and password"
#endif

#endif // BOARD_HAS_WIFI

#endif // KORRA_CONFIGS_WIFI_H
