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
    // client.setUpdateInterval(60 * 60 * 1000); // 60 min (default is 60 seconds)
}

void KorraTime::sync()
{
    if (client.update())
    {
        time_t epoch = client.getEpochTime();
        struct timeval tv = {
            .tv_sec = epoch,
            .tv_usec = 0, // do not set, it tends to cause errors
        };
        settimeofday(&tv, /* timezone */ nullptr);

        struct tm tm;
        time_t now = time(NULL);
        gmtime_r(&now, &tm);
        char time_str[sizeof("1970-01-01T00:00:00")];
        strftime(time_str, sizeof(time_str), "%FT%T", &tm);
        Serial.printf("Time is now %s\n", time_str);
    }
}
