/* croOS stdlib.c - Standard library implementation
 * qsort, bsearch, abs, labs, strtol, strtoul, atof, exit, atexit, system. */

#include "kernel/types.h"
#include "string.h"
#include "drivers/vga.h"
#include "mm/kmalloc.h"
#include "kernel/process.h"

long labs(long x) { return x < 0 ? -x : x; }

/* Quicksort */
static void swap(char *a, char *b, size_t width) {
    for (size_t i = 0; i < width; i++) {
        char tmp = a[i];
        a[i] = b[i];
        b[i] = tmp;
    }
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
    if (nmemb <= 1) return;

    /* Simple selection sort (stable, O(n^2) but small datasets are fine) */
    char *arr = (char*)base;
    for (size_t i = 0; i < nmemb - 1; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < nmemb; j++) {
            if (compar(arr + j * size, arr + min * size) < 0)
                min = j;
        }
        if (min != i)
            swap(arr + i * size, arr + min * size, size);
    }
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) {
    size_t lo = 0, hi = nmemb;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        int cmp = compar(key, (const char*)base + mid * size);
        if (cmp == 0) return (char*)base + mid * size;
        if (cmp < 0) hi = mid;
        else lo = mid + 1;
    }
    return NULL;
}

/* String to number conversions */
long strtol(const char *nptr, char **endptr, int base) {
    long result = 0;
    int neg = 0;

    while (*nptr == ' ' || *nptr == '\t') nptr++;
    if (*nptr == '-') { neg = 1; nptr++; }
    else if (*nptr == '+') nptr++;

    if (base == 0) {
        if (*nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) { base = 16; nptr += 2; }
        else if (*nptr == '0') { base = 8; }
        else { base = 10; }
    } else if (base == 16 && *nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) {
        nptr += 2;
    }

    while (*nptr) {
        int digit;
        if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'f') digit = 10 + *nptr - 'a';
        else if (*nptr >= 'A' && *nptr <= 'F') digit = 10 + *nptr - 'A';
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        nptr++;
    }

    if (endptr) *endptr = (char*)nptr;
    return neg ? -result : result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}

double atof(const char *s) {
    double result = 0.0;
    int neg = 0;

    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    while (*s >= '0' && *s <= '9')
        result = result * 10.0 + (*s++ - '0');

    if (*s == '.') {
        s++;
        double fraction = 0.1;
        while (*s >= '0' && *s <= '9') {
            result += (*s++ - '0') * fraction;
            fraction *= 0.1;
        }
    }
    return neg ? -result : result;
}

/* Process exit */
void exit(int status) {
    vga_puts("\n[exit(");
    char buf[12];
    itoa(status, buf, 10);
    vga_puts(buf);
    vga_puts(")]\n");
    /* TODO: kill current process */
    while(1) asm volatile("hlt");
}

void abort(void) {
    vga_puts("\n[abort]\n");
    while(1) asm volatile("hlt");
}

int system(const char *cmd) {
    vga_puts("[system] ");
    vga_puts(cmd);
    vga_putchar('\n');
    return -1;
}

void *malloc(size_t size) { return kmalloc(size); }
void *calloc(size_t n, size_t size) { return kcalloc(n, size); }
void *realloc(void *ptr, size_t size) { return krealloc(ptr, size); }
void  free(void *ptr) { kfree(ptr); }
