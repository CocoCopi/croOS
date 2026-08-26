// croOS freestanding time.h
#ifndef _CORROS_TIME_H
#define _CORROS_TIME_H
#include <stdint.h>
#define CLOCK_MONOTONIC 1
struct timespec { long tv_sec; long tv_nsec; };
int clock_gettime(int clk, struct timespec *ts);
#endif
