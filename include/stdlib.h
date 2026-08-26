// croOS freestanding stdlib.h
#ifndef _CORROS_STDLIB_H
#define _CORROS_STDLIB_H
#include <stddef.h>
void *malloc(size_t n);
void free(void *p);
void exit(int code);
#endif
