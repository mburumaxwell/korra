#ifndef KORRA_NETWORK_SHARED_H
#define KORRA_NETWORK_SHARED_H

#include <stddef.h>

#include <IPAddress.h>

// copied from https://github.com/zephyrproject-rtos/zephyr/issues/84960#issuecomment-2627029731
#define FMT_LL_ADDR_6 "%02X:%02X:%02X:%02X:%02X:%02X"
#define FMT_LL_ADDR_6_LOWER_NO_COLONS "%02x%02x%02x%02x%02x%02x"
#define PRINT_LL_ADDR_6(addr) addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#define KORRA_NETWORK_KIND_CELLULAR "cellular"
#define KORRA_NETWORK_KIND_ETHERNET "ethernet"
#define KORRA_NETWORK_KIND_WIFI "wifi"

struct korra_network_props {
  /** The kind of network. */
  char kind[sizeof(KORRA_NETWORK_KIND_CELLULAR) + 1];

  /**
   * The MAC address of the device.
   *
   * @note WiFi is 6 bytes but cellular may add 2 more.
   */
  uint8_t mac_addr[8];

  /**
   * The MAC address of the device formatted.
   *
   * @note longest is XX:XX:XX:XX:XX:XX:XX:XX
   */
  char mac[23 + 1];

  /**
   * The name of the network.
   *
   * @note WiFi SSID is 32 max, other are less or not there
   */
  char name[32 + 1];

  /** The local IP address. */
  IPAddress local_ipaddr;

  /**
   * The local IP address in string format.
   * @note IPv4 longest is 15 char -> 255.255.255.255
   * @note IPv6 longest is 45 char -> ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255
   */
  char local_ip[45 + 1];

  /**
   * The hostname of the device.
   * @note format is: "korra-{mac-without-separators}" e.g. korra-0123456789AB
   */
  char hostname[22 + 1];
};

#endif // KORRA_NETWORK_SHARED_H
