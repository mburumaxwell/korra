#ifdef CONFIG_BOARD_HAS_WIFI

#ifndef KORRA_WIFI_H_
#define KORRA_WIFI_H_

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

/** Initialize the WiFi logic */
extern void korra_wifi_init();

/** Connect to the configured WiFi network */
extern int korra_wifi_connect();

extern int korra_wifi_status(struct wifi_iface_status *status);
extern int korra_wifi_scan(k_timeout_t timeout);

#ifdef CONFIG_WIFI_SCAN_NETWORKS
extern int korra_wifi_scan(k_timeout_t timeout);
#endif // CONFIG_WIFI_SCAN_NETWORKS

#endif // KORRA_WIFI_H_

#endif // CONFIG_BOARD_HAS_WIFI
