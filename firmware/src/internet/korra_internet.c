#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_internet, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include "korra_internet.h"

#ifdef CONFIG_BOARD_HAS_CELLULAR
#include "korra_cellular.h"
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
#include "korra_ethernet.h"
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
#include "korra_wifi.h"
#endif // CONFIG_BOARD_HAS_WIFI

static struct net_mgmt_event_callback network_cb;
static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

void korra_internet_init()
{
	LOG_DBG("Initializing");

	// Register callbacks for connection manager
	net_mgmt_init_event_callback(&network_cb,
								 network_event_handler,
								 NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&network_cb);

#ifdef CONFIG_BOARD_HAS_CELLULAR
	korra_cellular_init();
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_ETHERNET
	korra_ethernet_init();
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
	korra_wifi_init();
#endif // CONFIG_BOARD_HAS_WIFI
}

int korra_internet_connect()
{
	int ret = 0;

#ifdef CONFIG_BOARD_HAS_CELLULAR
	ret = korra_cellular_connect();
	if (ret)
	{
		return ret;
	}
#endif // CONFIG_BOARD_HAS_CELLULAR

#ifdef CONFIG_BOARD_HAS_WIFI
	// This delay is to allow some drivers/modules to initialize (e.g. WPA_SUPPLICANT)
	// Basically waiting until we see this log ---> <inf> wifi_supplicant: wpa_supplicant initialized
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	k_sleep(K_SECONDS(1));
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT

#ifdef CONFIG_WIFI_SCAN_NETWORKS
	ret = korra_wifi_scan(K_FOREVER);
	if (ret)
	{
		return ret;
	}
#endif // CONFIG_WIFI_SCAN_NETWORKS

#ifdef CONFIG_WIFI_RUN_DPP
	ret = korra_wifi_provisioning();
	if (ret)
	{
		return ret;
	}
#endif // CONFIG_WIFI_RUN_DPP

	ret = korra_wifi_connect();
	if (ret)
	{
		return ret;
	}

#endif // CONFIG_BOARD_HAS_WIFI

	return ret;
}

static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED)
	{
		LOG_INF("Network connectivity gained!");
	}
	else if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
	{
		LOG_INF("Network connectivity lost!");
		// for cellular we need to restart the connection?
	}
}
