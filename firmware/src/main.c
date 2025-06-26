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
	uint8_t raw_devid[16] = {0};
	ssize_t raw_devid_len = hwinfo_get_device_id(raw_devid, sizeof(raw_devid));
	if (raw_devid_len <= 0)
	{
		LOG_WRN("Unable to get device ID: %d", raw_devid_len);
		return raw_devid_len;
	}

	// Generate the string version of the device id, no dashes so that it is easy to copy from terminal
	size_t devid_len = (raw_devid_len * 2) + 1;
	char devid[devid_len];
	for (size_t i = 0; i < raw_devid_len; ++i)
	{
		sprintf(devid + (i * 2), "%02x", raw_devid[i]);
	}
	devid_len--;
	printk("*** Device ID: %s (%d raw bytes) ***\n", devid, raw_devid_len);

	// Prepare credentials used in the system
	// korra_credentials_clear();
	korra_credentials_init(devid, (size_t)devid_len);

	// Print the device certificate for ease with provisioning
	size_t devcert_len = korra_credentials_get_device_cert(NULL, 0) + 1;
	if (devcert_len > 1)
	{
		char *devcert = k_malloc(devcert_len);
		devcert_len = korra_credentials_get_device_cert(devcert, devcert_len);
		printk("Device certificate to use for provisioning:\n%s\n", devcert);
		k_free(devcert);
	}
	else
	{
		LOG_WRN("Seems the device certificate for provisioning does not exist. Did you forget to call korra_credentials_init(...)?");
	}

	korra_time_init();
	korra_sensors_init();
	korra_internet_init();
	korra_cloud_init(devid, (size_t)devid_len);

	korra_internet_connect();

	k_sleep(K_FOREVER);
	return 0;
}
