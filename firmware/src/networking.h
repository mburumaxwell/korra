#ifndef NETWORKING_H_
#define NETWORKING_H_

#include <zephyr/kernel.h>

extern int networking_init();

/** Semaphore that indicates internet availability. */
extern struct k_sem sem_internet;

#endif // NETWORKING_H_
