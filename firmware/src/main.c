#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/hostname.h>
#include <app_version.h>

#include "korra_internet.h"

int main(void)
{
	// Log application version matching Kernel: *** Booting Zephyr OS build v4.1.0 ***
	printk("*** Booting Korra %s build v%s (%s) ***\n", CONFIG_APP_NAME, APP_VERSION_STRING, STRINGIFY(APP_BUILD_VERSION));
	printk("*** Running Board %s at %d MHz ***\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));
	printk("*** Hostname: %s ***\n", net_hostname_get());

	korra_internet_init();
	korra_internet_connect();

	LOG_INF("Waiting for internet...");
	k_sem_take(&network_connected, K_FOREVER);
	LOG_INF("Internet is ready");

	k_sleep(K_FOREVER);
	return 0;
}
