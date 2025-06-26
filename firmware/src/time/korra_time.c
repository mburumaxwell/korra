#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_time, LOG_LEVEL_INF);

#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/clock.h>

#include <korra_events.h>
#include <korra_net_utils.h>

#include "korra_time.h"

#define SYNC_SERVER_ADDRESS     CONFIG_SNTP_SERVER_ADDRESS
#define SYNC_SERVER_PORT        CONFIG_SNTP_SERVER_PORT
#define SYNC_SERVER_TIMEOUT     CONFIG_SNTP_SERVER_TIMEOUT_SECONDS * MSEC_PER_SEC
#define SYNC_INITIAL_DELAY      K_SECONDS(5)
#define SYNC_PERIOD             K_HOURS(6)
#define SYNC_RESCHEDULE_DELAY   K_SECONDS(30)

// Time sync is done using the configured time servers
// Some logic is pulled from
// - https://github.com/zephyrproject-rtos/zephyr/tree/8144a6638ac1568c32b6a8a86ed81a3ec18bb992/samples/net/sockets/sntp_client
// - https://github.com/zephyrproject-rtos/zephyr/blob/e92468c7a6be752e2f658efe073af49306658c50/subsys/shell/modules/date_service.c
//
// This implementation uses a timer and delayable work item.
// - The timer handles more logic that needs to happen every 6 hours but initially in the first 5 seconds.
// - When the timer fires, it places work in the work item which handles the actual sync.
// - Should it fail, the work item reschedules work in 30 seconds until it works instead of waiting for 6 hours.
// - When it works, the timer schedule is then maintained.
//
// Time sync requires internet so we rely on L4 callbacks from connectivity manager whish works irrespective of the network in use.
// Should internet fail, the timer is stopped then restarted when internet resumes.

static struct net_mgmt_event_callback l4_cb;
static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

static struct k_timer sync_timer;
static struct k_work_delayable sync_time_work;
static void sync_timer_handler(struct k_timer *timer);
static void sync_time_work_handler(struct k_work *work);

void korra_time_init()
{
    LOG_DBG("Initializing");

    /* Register callbacks for connection manager */
    net_mgmt_init_event_callback(&l4_cb,
                                 l4_event_handler,
                                 NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
    net_mgmt_add_event_callback(&l4_cb);

    // Initialize timer and work item for doing sync work
    k_timer_init(&sync_timer, sync_timer_handler, NULL);
    k_work_init_delayable(&sync_time_work, sync_time_work_handler);
}

static int sync_time()
{
    LOG_INF("Syncing ...");

    // resolve IP address
    struct sockaddr addr;
    socklen_t addrlen;
    int ret = korra_resolve_address(SYNC_SERVER_ADDRESS, SYNC_SERVER_PORT, AF_INET, SOCK_DGRAM, &addr, &addrlen);
    if (ret)
    {
        LOG_ERR("Failed to resolve address: %d", ret);
        return ret;
    }

    // initialize the context with the address
    struct sntp_ctx ctx;
    ret = sntp_init(&ctx, &addr, addrlen);
    if (ret)
    {
        LOG_ERR("Failed to init SNTP ctx: %d", ret);
        sntp_close(&ctx);
        return ret;
    }

    // query for the time
    struct sntp_time s_time;
    LOG_DBG("Sending SNTP request...");
    ret = sntp_query(&ctx, SYNC_SERVER_TIMEOUT, &s_time);
    if (ret)
    {
        LOG_ERR("SNTP request failed: %d", ret);
        sntp_close(&ctx);
        return ret;
    }

    // at this point we have received time
    LOG_INF("SNTP Time: %llu", s_time.seconds);
    sntp_close(&ctx);

    // update the system clock so that everyone can get what they need
    struct timespec ts = {
        .tv_sec = s_time.seconds,
        .tv_nsec = 0, // do not set to s_time.fraction, it tends to cause errors
    };
    ret = sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
    if (ret)
    {
        LOG_ERR("Could not set time %d", ret);
        return -EINVAL;
    }

    // print the time
    struct tm tm;
    gmtime_r(&ts.tv_sec, &tm);
    char time_str[sizeof("1970-01-01T00:00:00")];
    strftime(time_str, sizeof(time_str), "%FT%T", &tm);
    LOG_INF("Time is now %s", time_str);

    // notify those waiting
    korra_emit_event(KORRA_EVENT_TIME_SYNCED);

    return 0;
}

static void sync_time_work_handler(struct k_work *work)
{
    int ret = sync_time();
    if (ret)
    {
        k_work_reschedule(&sync_time_work, SYNC_RESCHEDULE_DELAY);
    }
}

static void sync_timer_handler(struct k_timer *timer)
{
    k_work_schedule(&sync_time_work, K_NO_WAIT);
}

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_L4_CONNECTED)
    {
        LOG_INF("Yay! We have internet!");
        k_timer_start(&sync_timer, SYNC_INITIAL_DELAY, SYNC_PERIOD);
    }
    else if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
    {
        LOG_INF("Oops! We lost connectivity to the internet");
        k_timer_stop(&sync_timer);
    }
}
