#ifndef CONFIG_H
#define CONFIG_H

#define STRINGIFY(a) __STRINGIFY(a)

#ifdef ARDUINO_ARCH_ESP32
#define CONFIG_BOARD_HAS_WIFI 1
#endif

// General Network
#ifdef CONFIG_BOARD_HAS_WIFI // || CONFIG_BOARD_HAS_ETHERNET || CONFIG_BOARD_HAS_CELLULAR
#define CONFIG_BOARD_HAS_INTERNET 1
#endif

// Functions that may exist in C++ but not in C but for consistency we use macros
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif // MAX
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif // MIN
#ifndef CLAMP
#define CLAMP(val, low, high) (((val) <= (low)) ? (low) : MIN(val, high))
#endif // CLAMP

#endif // CONFIG_H
