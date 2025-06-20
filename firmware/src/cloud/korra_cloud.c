#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_cloud, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>

#include "korra_cloud.h"

void korra_cloud_init(uint8_t *id, size_t id_len)
{
    LOG_DBG("Initializing");

    // TODO: register callbacks, allocate buffers, read cloud credentials here, setup clients
}
