#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_sensors, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>

#include "korra_sensors.h"

#define READ_PERIOD K_SECONDS(CONFIG_SENSORS_READ_PERIOD_SECONDS)

static struct k_timer collect_data_timer;
static struct k_work_delayable collect_data_work;
static void collect_data_timer_handler(struct k_timer *timer);
static void collect_data_work_handler(struct k_work *work);

void korra_sensors_init()
{
    LOG_DBG("Initializing");

    // TODO: setup sensors here (checking if the devices are ready and any specific setup)

    // Initialize timer and work item for data collection
    k_timer_init(&collect_data_timer, collect_data_timer_handler, NULL);
    k_work_init_delayable(&collect_data_work, collect_data_work_handler);

    // Start the timer
    k_timer_start(&collect_data_timer, READ_PERIOD, READ_PERIOD); // should we start with K_NO_WAIT instead?
}

static void collect_data_work_handler(struct k_work *work)
{
    LOG_INF("Collecting data");

    // TODO: collect actual sensors data
}

static void collect_data_timer_handler(struct k_timer *timer)
{
    k_work_schedule(&collect_data_work, K_NO_WAIT);
}
