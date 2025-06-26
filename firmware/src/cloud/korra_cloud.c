/*
 * korra_cloud.c
 *
 * Handles Azure DPS provisioning and IoT Hub connectivity over MQTT.
 *
 * Flow:
 * - Wait for network connection (via conn_mgr)
 * - If no DPS provisioning info, start provisioning
 * - Store assigned hub/device info in settings
 * - Connect to IoT Hub and maintain MQTT session
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_cloud, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/socket.h>

#include <korra_events.h>

#include "korra_cloud.h"
#include "korra_cloud_provisioning.h"

#define CLOUD_THREAD_STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(cloud_stack, CLOUD_THREAD_STACK_SIZE);
static struct k_thread cloud_thread;
static k_tid_t cloud_tid;
static void cloud_thread_start(void *arg_1, void *arg_2, void *arg_3);

/* Network event callback, semaphore, & handler */
static struct net_mgmt_event_callback network_cb;
static K_SEM_DEFINE(network_connected, 0, 1);
static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

void korra_cloud_init(const char *regid, const size_t regid_len)
{
	LOG_DBG("Initializing");

	// Register callbacks for connection manager
	net_mgmt_init_event_callback(&network_cb,
								 network_event_handler,
								 NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&network_cb);

	// Start the thread
	cloud_tid = k_thread_create(&cloud_thread,						// Thread struct
								cloud_stack,						// Stack
								K_THREAD_STACK_SIZEOF(cloud_stack), // Stack size
								cloud_thread_start,					// Entry point
								(void *)regid,						// arg_1
								(void *)&regid_len,					// arg_2
								NULL,								// arg_3
								7,									// Priority
								0,									// Options
								K_NO_WAIT);							// Delay
}

static void cloud_thread_start(void *arg_1, void *arg_2, void *arg_3)
{
	const char *regid = (const char *)arg_1;
	const size_t regid_len = *((const size_t *)arg_2);
	struct korra_cloud_provisioning_info prov = {0};
	int ret = korra_cloud_provisioning_init(regid, regid_len, &prov);
	if (ret)
	{
		LOG_ERR("Unable to initialise provisioning: %d", ret);
		return;
	}

	LOG_INF("Waiting for internet...");
	k_sem_take(&network_connected, K_FOREVER);
	LOG_INF("Internet is ready");

	LOG_INF("Waiting for time sync ...");
	korra_wait_for_event(KORRA_EVENT_TIME_SYNCED, K_FOREVER);
	LOG_INF("Time synced");

	while (1)
	{
		if (!prov.valid)
		{
			ret = korra_cloud_provisioning_run();
		}
		while (1)
		{
			// TODO: complete this logic
			k_sleep(K_FOREVER);
		}
	}
}

static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED)
	{
		k_sem_give(&network_connected);
		// TODO: setup client stuff

		return;
	}
	else if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
	{
		k_sem_take(&network_connected, K_FOREVER);
		// TODO: reset client stuff

		return;
	}
}

