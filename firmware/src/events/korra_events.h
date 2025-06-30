#ifndef KORRA_EVENTS_H_
#define KORRA_EVENTS_H_

#include <zephyr/kernel.h>

enum korra_app_event_type {
  KORRA_EVENT_TIME_SYNCED = BIT(0),
};

extern struct k_event korra_events;

static inline void korra_emit_event(uint32_t event) {
  k_event_post(&korra_events, event);
}

static inline void korra_wait_for_event(uint32_t mask, k_timeout_t timeout) {
  k_event_wait(&korra_events, mask, false, timeout);
}

#endif // KORRA_EVENTS_H_
