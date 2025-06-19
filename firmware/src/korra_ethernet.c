#ifdef CONFIG_BOARD_HAS_ETHERNET
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_ethernet, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet_mgmt.h>

#include "korra_utils.h"
#include "korra_ethernet.h"

static struct net_mgmt_event_callback ethernet_cb;

static void ethernet_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

void korra_ethernet_init()
{
    LOG_DBG("Initializing");

    // Initialize and add event callbacks for ethernet
    net_mgmt_init_event_callback(&ethernet_cb,
                                 ethernet_event_handler,
                                 NET_EVENT_ETHERNET_CARRIER_ON | NET_EVENT_ETHERNET_CARRIER_OFF);
    net_mgmt_add_event_callback(&ethernet_cb);
}

static void ethernet_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_ETHERNET_CARRIER_ON)
    {
        LOG_INF("Connected!");
    }
    else if (mgmt_event == NET_EVENT_ETHERNET_CARRIER_OFF)
    {
        LOG_INF("Disconnected!");
    }
}

#endif // CONFIG_BOARD_HAS_ETHERNET
