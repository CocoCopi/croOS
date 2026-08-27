/* croOS timer.h — Timer driver header */
#ifndef _TIMER_H
#define _TIMER_H

#include "kernel/types.h"

void     timer_init(uint32_t freq);
void     timer_sleep(uint32_t ms);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
uint64_t timer_tsc(void);

#endif
