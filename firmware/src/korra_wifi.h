#if CONFIG_WIFI

#ifndef KORRA_WIFI_H_
#define KORRA_WIFI_H_

/** Initialize the WiFi logic */
extern void korra_wifi_init();

/** Connect to the configured WiFi network */
extern int korra_wifi_connect();

#endif // KORRA_WIFI_H_

#endif // CONFIG_WIFI
