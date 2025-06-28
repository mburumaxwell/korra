#ifndef KORRA_CONFIGS_WIFI_H
#define KORRA_CONFIGS_WIFI_H

#ifdef ARDUINO_ARCH_ESP32
#define CONFIG_BOARD_HAS_WIFI 1
#endif

#ifdef CONFIG_BOARD_HAS_WIFI
// The amount of time to wait for a WiFi connection before rebooting
#define CONFIG_WIFI_CONNECTION_REBOOT_TIMEOUT_SEC 30

#if (defined(WIFI_ENTERPRISE_USERNAME) && !defined(WIFI_ENTERPRISE_PASSWORD)) || \
    (!defined(WIFI_ENTERPRISE_USERNAME) && defined(WIFI_ENTERPRISE_PASSWORD))
#error "Enterprise WIFI (802.1x) requires both username and password"
#endif

#endif // BOARD_HAS_WIFI

#endif // KORRA_CONFIGS_WIFI_H
