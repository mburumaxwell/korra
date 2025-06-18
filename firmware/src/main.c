#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <app_version.h>
#include "networking.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

int main(void)
{
	// Log application version matching Kernel: *** Booting Zephyr OS build v4.1.0 ***
	printk("*** Booting Korra %s build v%s ***\n", CONFIG_APP_NAME, APP_VERSION_STRING);

	// This delay is to allow some drivers/modules to initialize (e.g. WPA_SUPPLICANT)
	// Basically waiting until we see this log ---> <inf> wifi_supplicant: wpa_supplicant initialized
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	k_sleep(K_SECONDS(1));
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT

	if (networking_init()) {
		LOG_ERR("Networking initialization failed!");
		return 0;
	}

	k_sem_take(&sem_internet, K_FOREVER);
	LOG_INF("Internet is ready");

	k_sleep(K_FOREVER);
	return 0;
}
