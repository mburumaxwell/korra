#include "korra_time.h"

#define SYNC_SERVER_ADDRESS CONFIG_SNTP_SERVER_ADDRESS
#define SYNC_SERVER_PORT 123

KorraTime::KorraTime(UDP &client) : client(client, SYNC_SERVER_ADDRESS) {
}

KorraTime::~KorraTime() {
}

void KorraTime::begin(uint32_t update_interval_sec) {
  client.begin(SYNC_SERVER_PORT);
  client.setUpdateInterval(update_interval_sec * 1000);
}

void KorraTime::maintain() {
  if (!client.update()) return;

  time_t epoch = client.getEpochTime();
  struct timeval tv = {
      .tv_sec = epoch,
      .tv_usec = 0, // do not set, it tends to cause errors
  };
  settimeofday(&tv, /* timezone */ NULL);

  struct tm tm;
  time_t now = time(NULL);
  gmtime_r(&now, &tm);
  char time_str[sizeof("1970-01-01T00:00:00")];
  strftime(time_str, sizeof(time_str), "%FT%T", &tm);
  Serial.printf("Time is now %s\n", time_str);
}
