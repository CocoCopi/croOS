// croOS freestanding runtime — provides libc symbols for corros --compile output
#include "time.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// ── VGA text mode (0xB8000) ──────────────────────────────────────────────
#define VGA_ADDR 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25

static int vga_row = 0;
static int vga_col = 0;
static uint8_t vga_attr = 0x07;

static void vga_putchar(char c) {
    volatile uint8_t *vga = (volatile uint8_t *)VGA_ADDR;
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_ROWS) vga_row = 0;
        return;
    }
    int idx = (vga_row * VGA_COLS + vga_col) * 2;
    vga[idx] = (uint8_t)c;
    vga[idx + 1] = vga_attr;
    vga_col++;
    if (vga_col >= VGA_COLS) {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_ROWS) vga_row = 0;
    }
}

static void vga_puts(const char *s) {
    while (*s) vga_putchar(*s++);
}

// ── Bump allocator ───────────────────────────────────────────────────────
static uint8_t *heap_ptr = (uint8_t *)0x200000;

void *malloc(size_t n) {
    void *p = (void *)heap_ptr;
    heap_ptr += (n + 15) & ~15;
    return p;
}

void free(void *p) { (void)p; }

// ── String functions ─────────────────────────────────────────────────────
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

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    while (n--) *d++ = (uint8_t)c;
    return dst;
}

// ── Math ─────────────────────────────────────────────────────────────────
double fabs(double x) { return (x < 0) ? -x : x; }

double floor(double x) {
    int i = (int)x;
    return (x < 0 && x != (double)i) ? (double)(i - 1) : (double)i;
}

// ── Helper: print integer to VGA ─────────────────────────────────────────
static void print_ll(long long n) {
    if (n < 0) { vga_putchar('-'); n = -n; }
    char buf[32];
    int i = 0;
    if (n == 0) { vga_putchar('0'); return; }
    while (n > 0) { buf[i++] = '0' + (int)(n % 10); n /= 10; }
    while (i > 0) vga_putchar(buf[--i]);
}

// ── sprintf (used by c_fmt_num in the generated code) ────────────────────
// The corros compiler generates: sprintf(buf, "%lld", (long long)x);
// and:                           sprintf(buf, "%.15g", x);
int sprintf(char *buf, const char *fmt, ...) {
    // Minimal: only handle %lld and %.15g for c_fmt_num
    va_list ap;
    va_start(ap, fmt);
    int pos = 0;
    const char *p = fmt;
    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == 'l' && *(p+1) == 'l' && *(p+2) == 'd') {
                long long v = va_arg(ap, long long);
                if (v < 0) { buf[pos++] = '-'; v = -v; }
                if (v == 0) { buf[pos++] = '0'; }
                else {
                    char tmp[32]; int ti = 0;
                    while (v > 0) { tmp[ti++] = '0' + (int)(v % 10); v /= 10; }
                    while (ti > 0) buf[pos++] = tmp[--ti];
                }
                p += 3;
            } else if (*p == '.' || *p == 'l' || *p == 'd' || *p == 'g' || *p == 'f') {
                // Skip format specifier for unsupported types
                while (*p && *p != 'd' && *p != 'g' && *p != 'f' && *p != 's') p++;
                if (*p) p++;
            } else {
                p++;
            }
        } else {
            buf[pos++] = *p++;
        }
    }
    buf[pos] = 0;
    va_end(ap);
    return pos;
}

// ── printf (the main output function used by corros --compile) ────────────
// The generated code always calls: printf("%s\n", str);
// for speak() and similar. Sometimes printf("%lld\n", num) for str().
int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *p = fmt;
    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == 's') {
                const char *s = va_arg(ap, const char *);
                vga_puts(s ? s : "(null)");
                p++;
            } else if (*p == 'l' && *(p+1) == 'l' && *(p+2) == 'd') {
                long long v = va_arg(ap, long long);
                print_ll(v);
                p += 3;
            } else if (*p == '.') {
                // Skip float format like %.15g — just consume the next arg
                while (*p && *p != 'g' && *p != 'f') p++;
                if (*p) p++;
                // Consume the double argument
                (void)va_arg(ap, double);
            } else {
                p++;
            }
        } else if (*p == '\\') {
            p++;
            if (*p == 'n') { vga_putchar('\n'); p++; }
            else if (*p == '\\') { vga_putchar('\\'); p++; }
            else if (*p == '0') { p++; }
            else { vga_putchar('\\'); vga_putchar(*p); p++; }
        } else {
            vga_putchar(*p);
            p++;
        }
    }
    va_end(ap);
    return 0;
}

int fprintf(void *stream, const char *fmt, ...) {
    (void)stream; (void)fmt;
    return 0;
}

void exit(int code) {
    (void)code;
    while (1) { __asm__ volatile("hlt"); }
}

// ── c_tick (used by tick() in generated code) ────────────────────────────
// In freestanding mode, read the TSC
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// PIT runs at ~1.193 MHz, so TSC / 1193182 ≈ milliseconds
// But we don't know the actual TSC frequency; approximate with a loop counter.
static volatile uint64_t pit_ticks = 0;
void timer_irq_handler(void) { pit_ticks++; }

// Approximate tick in milliseconds using TSC
double c_tick_approx(void) {
    // Use PIT ticks if IRQ handler is set up
    return (double)pit_ticks * 8.0; // PIT fires every ~8ms at 119 Hz
}

// ── clock_gettime (needed by generated c_tick) ───────────────────────────
int clock_gettime(int clk, struct timespec *ts) {
    (void)clk;
    // Use a simple loop counter as a fake clock
    static volatile uint64_t counter = 0;
    counter += 1193; // pretend ~1ms passes per call
    ts->tv_sec = (long)(counter / 1000000);
    ts->tv_nsec = (long)((counter % 1000000) * 1000);
    return 0;
}

// ── _start (entry point for the kernel) ──────────────────────────────────
extern void kernel_main(void);
void _start(void) {
    kernel_main();
    while (1) { __asm__ volatile("hlt"); }
}
