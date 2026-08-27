/* croOS types.h — Fundamental types for the kernel */
#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef uint32_t size_t;
typedef int32_t  ssize_t;
typedef int32_t  pid_t;
typedef uint32_t addr_t;
typedef uint32_t uintptr_t;

#define NULL ((void*)0)
#define true  1
#define false 0
#define bool  uint8_t

#define SEMI true
#define __packed __attribute__((packed))
#define __aligned(x) __attribute__((aligned(x)))

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/* inline asm helpers */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void io_wait(void) { outb(0x80, 0); }
static inline void cli(void) { asm volatile("cli"); }
static inline void sti(void) { asm volatile("sti"); }
static inline void hlt(void) { asm volatile("hlt"); }

#endif
