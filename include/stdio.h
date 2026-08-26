// corrOS freestanding stdio.h
#ifndef _CORROS_STDIO_H
#define _CORROS_STDIO_H
#include <stddef.h>
int printf(const char *fmt, ...);
int fprintf(void *stream, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
#endif
