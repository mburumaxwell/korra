#ifdef CONFIG_BOARD_HAS_CELLULAR
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_cellular, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>

#include "korra_utils.h"
#include "korra_cellular.h"

#TODO : find better interface here
#define get_cellular_iface net_if_get_default

static struct net_mgmt_event_callback cellular_cb;

static void cellular_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

void korra_cellular_init()
{
    LOG_DBG("Initializing");

    struct net_if *iface = get_cellular_iface();
    struct net_linkaddr *linkaddr = net_if_get_link_addr(iface);
    if (linkaddr && linkaddr->len == NET_ETH_ADDR_LEN)
    {
        LOG_INF("Mac Address: " FMT_LL_ADDR_6, PRINT_LL_ADDR_6(linkaddr->addr));
    }

    // Initialize and add event callbacks for cellular
    net_mgmt_init_event_callback(&cellular_cb,
                                 cellular_event_handler,
                                 NET_EVENT_CELLULAR_CONNECTED | NET_EVENT_CELLULAR_DISCONNECTED);
    net_mgmt_add_event_callback(&cellular_cb);
}

static void cellular_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_CELLULAR_CONNECTED)
    {
        LOG_INF("Connected!");
    }
    else if (mgmt_event == NET_EVENT_CELLULAR_DISCONNECTED)
    {
        LOG_INF("Disconnected!");
    }
}

#endif // CONFIG_BOARD_HAS_CELLULAR
