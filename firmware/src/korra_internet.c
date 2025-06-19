#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_internet, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include "korra_internet.h"

#ifdef CONFIG_BOARD_HAS_ETHERNET
#include "korra_ethernet.h"
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
#include "korra_wifi.h"
#endif // CONFIG_BOARD_HAS_WIFI

#ifdef CONFIG_NET_CONNECTION_MANAGER
#define USING_CONNECTION_MANAGER
#endif // CONFIG_NET_CONNECTION_MANAGER

#ifdef USING_CONNECTION_MANAGER
#define NETWORK_EVENTS (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#else
#define NETWORK_EVENTS (NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL)
#endif // USING_CONNECTION_MANAGER

K_SEM_DEFINE(network_connected, 0, 1);

#ifdef USING_CONNECTION_MANAGER
static struct k_work_delayable network_reconnect_work;
static void network_reconnect_work_handler(struct k_work *work);
#endif // USING_CONNECTION_MANAGER

static struct net_mgmt_event_callback network_cb;
static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

void korra_internet_init()
{
	LOG_DBG("Setting up");

	// Register callbacks for connection manager or IP assignment
	net_mgmt_init_event_callback(&network_cb, network_event_handler, NETWORK_EVENTS);
	net_mgmt_add_event_callback(&network_cb);

#ifdef CONFIG_BOARD_HAS_ETHERNET
	korra_ethernet_init();
#endif // CONFIG_BOARD_HAS_ETHERNET

#ifdef CONFIG_BOARD_HAS_WIFI
	korra_wifi_init();

#ifdef USING_CONNECTION_MANAGER
	// Initialize work item for reconnection
	k_work_init_delayable(&network_reconnect_work, network_reconnect_work_handler);
#endif // USING_CONNECTION_MANAGER
}

int korra_internet_connect()
{
	int ret = 0;

#ifdef CONFIG_WIFI_SCAN_NETWORKS
	ret = korra_wifi_scan(K_FOREVER);
	if (ret)
	{
		return ret;
	}
#else
	// This delay is to allow some drivers/modules to initialize (e.g. WPA_SUPPLICANT)
	// Basically waiting until we see this log ---> <inf> wifi_supplicant: wpa_supplicant initialized
	// We only need this if we are not scanning because scanning does the delay already.
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	k_sleep(K_SECONDS(1));
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT
#endif // CONFIG_WIFI_SCAN_NETWORKS

	ret = korra_wifi_connect();
	if (ret)
	{
		return ret;
	}
#endif // CONFIG_BOARD_HAS_WIFI

	return 0;
}

static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
#ifdef USING_CONNECTION_MANAGER
	if (mgmt_event == NET_EVENT_L4_CONNECTED)
	{
		LOG_INF("Network connectivity gained!");
		k_sem_give(&network_connected);
	}
	else if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
	{
#ifdef CONFIG_INTERNET_RECONNECTION
		// Retry in 5 seconds
		LOG_INF("Network connectivity lost! Retrying in 5 sec ...");
		k_work_schedule(&network_reconnect_work, K_SECONDS(5));
		k_sem_take(&network_connected, K_FOREVER);
#endif // CONFIG_INTERNET_RECONNECTION
	}
#else
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD)
	{
		LOG_INF("Network connectivity gained!");
		k_sem_give(&network_connected);
	}
	else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL)
	{
		LOG_INF("Network connectivity lost!");
		k_sem_take(&network_connected, K_FOREVER);
		// reconnect handled by WiFi/ethernet/cellular
	}
#endif // USING_CONNECTION_MANAGER
}

#ifdef USING_CONNECTION_MANAGER
static void network_reconnect_work_handler(struct k_work *work)
{
	int ret = korra_internet_connect();
	if (ret)
	{
		// Schedule reconnection in 30 sec (longer to skip errors)
		k_work_schedule(&network_reconnect_work, K_SECONDS(30));
	}
}
#endif // USING_CONNECTION_MANAGER
