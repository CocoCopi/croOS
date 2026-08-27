/* croOS timer.c — Programmable Interval Timer (PIT) + TSC
 * PIT channel 0 fires IRQ0 at ~100 Hz for the scheduler tick.
 * TSC (rdtsc) provides sub-microsecond timing for benchmarks. */

#include "kernel/types.h"
#include "drivers/vga.h"
#include "kernel/idt.h"

static volatile uint32_t ticks = 0;
static volatile uint32_t sleep_ticks = 0;

uint32_t timer_get_ticks(void) { return ticks; }

uint32_t timer_get_seconds(void) { return ticks / 100; }

static void timer_irq(regs_t *regs) {
    (void)regs;
    ticks++;
    if (sleep_ticks > 0) sleep_ticks--;
}

uint64_t timer_tsc(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void timer_init(uint32_t freq) {
    ticks = 0;
    isr_install_handler(32, timer_irq);

    /* PIT Channel 0, mode 3 (square wave), divisor */
    uint32_t divisor = 1193180 / freq;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    /* Enable IRQ0 */
    outb(0x21, inb(0x21) & ~0x01);
}

void timer_sleep(uint32_t ms) {
    sleep_ticks = (ms * 100) / 1000;
    if (sleep_ticks == 0) sleep_ticks = 1;
    while (sleep_ticks > 0) asm volatile("hlt");
}
