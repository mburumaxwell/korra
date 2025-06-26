#ifndef KORRA_NET_UTILS_H_
#define KORRA_NET_UTILS_H_

#include <zephyr/net/socket.h>

extern int korra_resolve_address(const char *host, uint16_t port, int family, int socktype, struct sockaddr *addr, socklen_t *addrlen);

// copied from https://github.com/zephyrproject-rtos/zephyr/issues/84960#issuecomment-2627029731
#include <stdio.h>
#include <stdint.h>
#define FMT_LL_ADDR_6 "%02X:%02X:%02X:%02X:%02X:%02X"
#define PRINT_LL_ADDR_6(addr) addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#endif // KORRA_NET_UTILS_H_
