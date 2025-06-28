#ifndef CONFIG_H
#define CONFIG_H

#include "configs/korra_configs_cellular.h"
#include "configs/korra_configs_ethernet.h"
#include "configs/korra_configs_wifi.h"

#include "configs/korra_configs_sensors.h"

// General Network
#ifdef CONFIG_BOARD_HAS_WIFI // || BOARD_HAS_ETHERNET || BOARD_HAS_CELLULAR
#define CONFIG_BOARD_HAS_INTERNET 1
#endif

#endif // CONFIG_H
