// croOS freestanding string.h
#ifndef _CORROS_STRING_H
#define _CORROS_STRING_H
#include <stddef.h>
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
#endif
