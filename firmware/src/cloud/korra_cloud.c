#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_cloud, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include "korra_cloud.h"

#define CLOUD_THREAD_STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(cloud_stack, CLOUD_THREAD_STACK_SIZE);
static struct k_thread cloud_thread;
static k_tid_t cloud_tid;
static void cloud_thread_start(void *arg_1, void *arg_2, void *arg_3);

static K_SEM_DEFINE(network_connected, 0, 1);

static struct net_mgmt_event_callback network_cb;
static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);


void korra_cloud_init(uint8_t *id, size_t id_len)
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
								NULL,								// arg_1
								NULL,								// arg_2
								NULL,								// arg_3
								7,									// Priority
								0,									// Options
								K_NO_WAIT);							// Delay
}

static void cloud_thread_start(void *arg_1, void *arg_2, void *arg_3)
{
	LOG_INF("Waiting for internet...");
	k_sem_take(&network_connected, K_FOREVER);
	LOG_INF("Internet is ready");

	// TODO: allocate buffers, read cloud credentials here, setup clients

	while (1)
	{
		k_sleep(K_FOREVER);
	}
}

static void network_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED)
	{
		k_sem_give(&network_connected);
		// TODO: setup client stuff
	}
	else if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
	{
		k_sem_take(&network_connected, K_FOREVER);
		// TODO: reset client stuff
	}
}
