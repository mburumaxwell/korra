#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/net/hostname.h>

#include <app_version.h>
#include <korra_credentials.h>
#include <korra_internet.h>
#include <korra_cloud.h>
#include <korra_sensors.h>
#include <korra_time.h>

#ifndef CONFIG_ARM
#define SystemCoreClock CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif

int main(void)
{
	// Log application version matching Kernel: *** Booting Zephyr OS build v4.1.0 ***
	printk("*** Booting Korra %s build v%s (%s) ***\n", CONFIG_APP_NAME, APP_VERSION_STRING, STRINGIFY(APP_BUILD_VERSION));
	printk("*** Running Board %s at %d MHz ***\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));
	printk("*** Hostname: %s ***\n", net_hostname_get());

	uint32_t dev_id[4] = {0};
	int ret = hwinfo_get_device_id((uint8_t *)dev_id, sizeof(dev_id));
	if (ret)
	{
		printk("*** Device ID: %08x-%08x-%08x-%08x ***\n", dev_id[0], dev_id[1], dev_id[2], dev_id[3]);
	}
	else
	{
		LOG_WRN("Unable to get device ID: %d", ret);
	}

	korra_time_init();
	korra_credentials_init();
	korra_sensors_init();
	korra_internet_init();
	korra_cloud_init((uint8_t *)dev_id, sizeof(dev_id));

	korra_internet_connect();

	k_sleep(K_FOREVER);
	return 0;
}
