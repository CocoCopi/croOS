/* croOS string.c - C standard library string functions
 * strlen, strcpy, strncpy, strcmp, strncmp, strcat, strchr, strrchr,
 * memcpy, memmove, memset, memcmp, itoa, atoi, snprintf, vsnprintf */

#include "kernel/types.h"

size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *d = dst;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dst;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcat(char *dst, const char *src) {
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++));
    return dst;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (c == 0) ? (char*)s : NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == 0) return (char*)s;
    return (char*)last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    size_t nlen = strlen(needle);
    while (*haystack) {
        if (strncmp(haystack, needle, nlen) == 0) return (char*)haystack;
        haystack++;
    }
    return NULL;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *pa = a, *pb = b;
    while (n--) {
        if (*pa != *pb) return *pa - *pb;
        pa++; pb++;
    }
    return 0;
}

int itoa(int value, char *buf, int base) {
    char tmp[32];
    int neg = 0, i = 0;
    unsigned int u;
    if (value < 0 && base == 10) { neg = 1; u = (unsigned int)-value; }
    else u = (unsigned int)value;
    do {
        int d = u % base;
        tmp[i++] = d < 10 ? '0' + d : 'A' + d - 10;
        u /= base;
    } while (u > 0);
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
    return j;
}

int atoi(const char *s) {
    int n = 0, neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return neg ? -n : n;
}

/* Core vsnprintf - does all the work */
int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list args) {
    size_t pos = 0;
    const char *f = fmt;

    while (*f && pos < size - 1) {
        if (*f == '%' && *(f+1)) {
            f++;
            while (*f == '-' || *f == '+' || *f == ' ' || *f == '0' ||
                   (*f >= '1' && *f <= '9')) f++;
            if (*f == '*') { f++; /* skip width arg */ }

            char tmp[32];
            int len = 0;
            switch (*f) {
                case 'd': case 'i':
                    len = itoa(__builtin_va_arg(args, int), tmp, 10);
                    break;
                case 'u':
                    len = itoa((int)__builtin_va_arg(args, unsigned int), tmp, 10);
                    break;
                case 'x': case 'X': {
                    unsigned int v = __builtin_va_arg(args, unsigned int);
                    if (v == 0) { tmp[0] = '0'; len = 1; }
                    else {
                        char hex[] = "0123456789abcdef";
                        unsigned int tv = v; len = 0;
                        while (tv > 0) { len++; tv >>= 4; }
                        for (int i = len - 1; i >= 0; i--) {
                            tmp[i] = hex[v & 0xF]; v >>= 4;
                        }
                    }
                    break;
                }
                case 's': {
                    const char *s = __builtin_va_arg(args, const char*);
                    if (!s) s = "(null)";
                    len = strlen(s);
                    memcpy(tmp, s, len > 31 ? 31 : len);
                    if (len > 31) len = 31;
                    break;
                }
                case 'c':
                    tmp[0] = (char)__builtin_va_arg(args, int);
                    len = 1;
                    break;
                case '%':
                    tmp[0] = '%'; len = 1;
                    break;
                default:
                    tmp[0] = *f; len = 1;
                    break;
            }
            for (int i = 0; i < len && pos < size - 1; i++)
                buf[pos++] = tmp[i];
        } else {
            buf[pos++] = *f;
        }
        f++;
    }
    buf[pos] = '\0';
    return (int)pos;
}

/* snprintf wraps vsnprintf */
int snprintf(char *buf, size_t size, const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int ret = vsnprintf(buf, size, fmt, args);
    __builtin_va_end(args);
    return ret;
}
