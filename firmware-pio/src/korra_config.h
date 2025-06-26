#ifndef CONFIG_H
#define CONFIG_H

#include "configs/korra_configs_cellular.h"
#include "configs/korra_configs_ethernet.h"
#include "configs/korra_configs_wifi.h"

#include "configs/korra_configs_sensors.h"

// General Network
#if BOARD_HAS_WIFI // || BOARD_HAS_ETHERNET || BOARD_HAS_CELLULAR
#define BOARD_HAS_NETWORK 1
#else
#define BOARD_HAS_NETWORK 0
#endif

#if BOARD_HAS_NETWORK
#define MAC_ADDRESS_LENGTH 6
#endif

#endif // CONFIG_H
