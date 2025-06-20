#ifndef KORRA_INTERNET_H_
#define KORRA_INTERNET_H_

#include <zephyr/kernel.h>

/** Initialize the internet logic */
extern void korra_internet_init();

/** Connect to the internet */
extern int korra_internet_connect();

extern struct k_sem network_connected;

#endif // KORRA_INTERNET_H_
