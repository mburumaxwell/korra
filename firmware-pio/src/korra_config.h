#ifndef CONFIG_H
#define CONFIG_H

#ifdef ARDUINO_ARCH_ESP32
#define CONFIG_BOARD_HAS_WIFI 1
#endif

#ifdef CONFIG_BOARD_HAS_WIFI
// The amount of time to wait for a WiFi connection before rebooting
#define CONFIG_WIFI_CONNECTION_REBOOT_TIMEOUT_SEC 30
#endif // CONFIG_BOARD_HAS_WIFI

// General Network
#ifdef CONFIG_BOARD_HAS_WIFI // || CONFIG_BOARD_HAS_ETHERNET || CONFIG_BOARD_HAS_CELLULAR
#define CONFIG_BOARD_HAS_INTERNET 1
#endif

#endif // CONFIG_H
