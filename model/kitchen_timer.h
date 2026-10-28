#ifndef KITCHEN_TIMER_H
#define KITCHEN_TIMER_H

#include <stdint.h>

/* Time is supplied by the caller in milliseconds, independently of any UI. */
#define KITCHEN_TIMER_CAPACITY 3
typedef enum { KT_RUNNING = 0, KT_PAUSED = 1, KT_DONE = 2 } kt_status;
typedef struct {
    uint32_t id;
    int64_t total_ms, left_ms, deadline_ms;
    kt_status status;
    int acknowledged;
} kt_timer;
typedef struct {
    kt_timer items[KITCHEN_TIMER_CAPACITY];
    uint32_t count, next_id;
} kt_service;

void kt_init(kt_service *service);
/* Return 1 on success, 0 when seconds is zero, capacity is full, or time overflows. */
int kt_add(kt_service *service, uint32_t seconds, int64_t now, uint32_t *id);
int64_t kt_remaining(const kt_timer *timer, int64_t now);
int kt_tick(kt_service *service, int64_t now);
void kt_toggle(kt_service *service, uint32_t id, int64_t now);
void kt_cancel(kt_service *service, uint32_t id);
uint32_t kt_alarm(kt_service *service, int64_t now);
void kt_acknowledge(kt_service *service, uint32_t id);

#endif
