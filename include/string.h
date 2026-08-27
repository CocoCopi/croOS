/* croOS string.h — Standard string library declarations */
#ifndef _STRING_H
#define _STRING_H

#include "kernel/types.h"

size_t strlen(const char *s);
char   *strcpy(char *dst, const char *src);
char   *strncpy(char *dst, const char *src, size_t n);
int     strcmp(const char *a, const char *b);
int     strncmp(const char *a, const char *b, size_t n);
char   *strcat(char *dst, const char *src);
char   *strchr(const char *s, int c);
char   *strrchr(const char *s, int c);
char   *strstr(const char *haystack, const char *needle);

void   *memcpy(void *dst, const void *src, size_t n);
void   *memmove(void *dst, const void *src, size_t n);
void   *memset(void *s, int c, size_t n);
int     memcmp(const void *a, const void *b, size_t n);

int     itoa(int value, char *buf, int base);
int     atoi(const char *s);
int     snprintf(char *buf, size_t size, const char *fmt, ...);

#endif
