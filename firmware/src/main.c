#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include <app_version.h>

#if CONFIG_BOARD_HAS_ETHERNET
#include "korra_ethernet.h"
#endif // CONFIG_BOARD_HAS_ETHERNET

#if CONFIG_WIFI
#include "korra_wifi.h"
#endif // CONFIG_WIFI

K_SEM_DEFINE(network_connected, 0, 1);

int main(void)
{
#if CONFIG_BOARD_HAS_ETHERNET
	korra_ethernet_init();
#endif // CONFIG_BOARD_HAS_ETHERNET

#if CONFIG_WIFI
	korra_wifi_init();

	// This delay is to allow some drivers/modules to initialize (e.g. WPA_SUPPLICANT)
	// Basically waiting until we see this log ---> <inf> wifi_supplicant: wpa_supplicant initialized
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	k_sleep(K_SECONDS(1));
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT

	korra_wifi_connect();
#endif // CONFIG_WIFI

	LOG_INF("Waiting for internet...");
	k_sem_take(&network_connected, K_FOREVER);
	LOG_INF("Internet is ready");

	k_sleep(K_FOREVER);
	return 0;
}

static void network_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED)
	{
		k_sem_give(&network_connected);
		LOG_INF("Network connectivity gained!");
	}
	else if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
	{
		LOG_INF("Network connectivity lost!");
		k_sem_take(&network_connected, K_FOREVER);
	}
	else
	{
		// DHCPv* related events are only here because sometimes we lose network but want to see how the logs go
		// We would remove this later after we are sure we have things setup correctly for L4 above
		if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD)
		{
			LOG_DBG("IPv4 address added!");
		}
		else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL)
		{
			LOG_DBG("IPv4 address removed!");
		}
		else if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD)
		{
			LOG_DBG("IPv6 address added!");
		}
		else if (mgmt_event == NET_EVENT_IPV6_ADDR_DEL)
		{
			LOG_DBG("IPv6 address removed!");
		}
	}
}

static struct net_mgmt_event_callback l4_cb, ipv4_cb, ipv6_cb;
static int before_main(void)
{
	// Log application version matching Kernel: *** Booting Zephyr OS build v4.1.0 ***
	printk("*** Booting Korra %s build v%s (%s) ***\n", CONFIG_APP_NAME, APP_VERSION_STRING, STRINGIFY(APP_BUILD_VERSION));

	// Register callbacks for connection manager
	net_mgmt_init_event_callback(&l4_cb,
								 network_event_handler,
								 NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&l4_cb);

	// We are registering for DHCPv* events because sometimes, we lose internet and want to track this in the logs
	// We would remove this later after we are sure we have things setup correctly for L4 above

	// Register callbacks for IPv4 assignment (DHCPv4)
	net_mgmt_init_event_callback(&ipv4_cb,
								 network_event_handler,
								 NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&ipv4_cb);

	// Register callbacks for IPv6 assignment (DHCPv6)
	net_mgmt_init_event_callback(&ipv6_cb,
								 network_event_handler,
								 NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL);
	net_mgmt_add_event_callback(&ipv6_cb);

	return 0;
}

SYS_INIT(before_main, /* level */ APPLICATION, /* priority */ 0);
