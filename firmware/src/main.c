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

	// Most ARM devices have 96bit or 128bit ones but ESP32 has 64bit or less (when it is the Mac)
	// We allocate 16 byte buffer to accommodate for 128bit (128/8=16)
	uint8_t devid[16] = {0};
	ssize_t devid_len = hwinfo_get_device_id(devid, sizeof(devid));
	if (devid_len > 0)
	{
		// print just the length assigned, no dashes so that it is easy to copy from terminal
		printk("*** Device ID: ");
		for (uint8_t i = 0; i < (devid_len); i++) printk("%02x", devid[i]);
		printk(" (%d bytes) ***\n", devid_len);
	}
	else
	{
		LOG_WRN("Unable to get device ID: %d", devid_len);
	}

	korra_time_init();
	korra_credentials_init();
	korra_sensors_init();
	korra_internet_init();
	korra_cloud_init((uint8_t *)devid, sizeof(devid));

	korra_internet_connect();

	k_sleep(K_FOREVER);
	return 0;
}
