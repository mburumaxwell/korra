#include "korra_time.h"

KorraTime::KorraTime(UDP &udpClient) : client(udpClient, CONFIG_SNTP_SERVER_ADDRESS)
{
}

KorraTime::~KorraTime()
{
}

void KorraTime::begin()
{
    client.begin(CONFIG_SNTP_SERVER_PORT);
    // client.setUpdateInterval(5 * 60 * 1000); // 5 min (default is 60 seconds)
}

void KorraTime::maintain()
{
    client.update();
}

time_t KorraTime::now()
{
    return client.getEpochTime();
}

void KorraTime::now(struct tm *tm)
{
    time_t ts = now();
    gmtime_r(&ts, tm);
}
