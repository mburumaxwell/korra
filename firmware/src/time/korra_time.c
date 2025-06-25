#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_time, LOG_LEVEL_INF);

#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/posix/time.h>
#include <arpa/inet.h>

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
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
    };

    struct addrinfo *res = NULL;
    LOG_DBG("Fetching IPv4 for %s", SYNC_SERVER_ADDRESS);
    int ret = getaddrinfo(SYNC_SERVER_ADDRESS, NULL, &hints, &res);
    if (ret)
    {
        LOG_INF("getaddrinfo failed (%d, errno %d)", ret, errno);
        return ret;
    }

    // only interested in the first result
    // change type to one we can use then free up what was given
    struct sockaddr addr = *(res->ai_addr);
    socklen_t addrlen = res->ai_addrlen;
    freeaddrinfo(res);                                  // free the allocated memory
    net_sin(&addr)->sin_port = htons(SYNC_SERVER_PORT); // store the port

    // print out the resolved address
    char addr_str[INET6_ADDRSTRLEN] = {0};
    inet_ntop(addr.sa_family, &net_sin(&addr)->sin_addr, addr_str, sizeof(addr_str));
    LOG_DBG("%s -> %s", SYNC_SERVER_ADDRESS, addr_str);

    struct sntp_ctx ctx;
    ret = sntp_init(&ctx, &addr, addrlen);
    if (ret)
    {
        LOG_ERR("Failed to init SNTP ctx: %d", ret);
        sntp_close(&ctx);
        return ret;
    }

    struct sntp_time s_time;
    LOG_DBG("Sending SNTP request...");
    ret = sntp_query(&ctx, SYNC_SERVER_TIMEOUT, &s_time);
    if (ret)
    {
        LOG_ERR("SNTP request failed: %d", ret);
        sntp_close(&ctx);
        return ret;
    }

    LOG_INF("SNTP Time: %llu", s_time.seconds);
    sntp_close(&ctx);

    struct timespec ts = {
        .tv_sec = s_time.seconds,
        .tv_nsec = s_time.fraction,
    };
    ret = clock_settime(CLOCK_REALTIME, &ts);
    if (ret == -1) // on first time, this is likely transient
    {
        k_sleep(K_SECONDS(2));
        ret = clock_settime(CLOCK_REALTIME, &ts);
    }
    if (ret)
    {
        LOG_ERR("Could not set time %d", ret);
        return -EINVAL;
    }

    struct tm tm;
    gmtime_r(&ts.tv_sec, &tm);
    LOG_INF("Time is now %d-%02u-%02u %02u:%02u:%02u UTC",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec);

    return ret;
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
