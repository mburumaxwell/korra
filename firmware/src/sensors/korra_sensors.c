#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_sensors, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>

#include "korra_sensors.h"

#define DATA_COLLECTION_INTERVAL K_SECONDS(60)

static struct k_work_delayable collect_data_work;
static void collect_data_work_handler(struct k_work *work);
static void collect_data();

void korra_sensors_init()
{
    LOG_DBG("Initializing");

    // TODO: setup sensors here (checking if the devices are ready and any specific setup)

    // Initialize work item for collecting data and schedule first read
    k_work_init_delayable(&collect_data_work, collect_data_work_handler);
    k_work_schedule(&collect_data_work, DATA_COLLECTION_INTERVAL); // should this be K_NO_WAIT?
}

static void collect_data()
{
    LOG_INF("Collecting data");

    // TODO: collect actual sensors data
}

static void collect_data_work_handler(struct k_work *work)
{
    collect_data();
    k_work_schedule(&collect_data_work, DATA_COLLECTION_INTERVAL); // reschedule
}
