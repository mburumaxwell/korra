#ifndef KORRA_UTILS_H_
#define KORRA_UTILS_H_

// copied from https://github.com/zephyrproject-rtos/zephyr/issues/84960#issuecomment-2627029731
#include <stdio.h>
#include <stdint.h>
#define FMT_LL_ADDR_6 "%02X:%02X:%02X:%02X:%02X:%02X"
#define PRINT_LL_ADDR_6(addr) addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#endif // KORRA_UTILS_H_
