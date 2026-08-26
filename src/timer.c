/* croOS — PIT timer (PIT channel 0, ~100 Hz). */
#include <stdint.h>
#include "ports.h"

#define PIT_FREQ    1193182
#define PIT_RATE    100  /* ticks per second */

static volatile unsigned int ticks = 0;

void timer_init(void)
{
    uint16_t divisor = PIT_FREQ / PIT_RATE;
    outb(0x43, 0x36);           /* channel 0, lobyte/hibyte, rate gen */
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

/* Called by the PIT IRQ0 handler (or manually if no PIC is set up). */
void timer_tick(void) { ticks++; }

unsigned int timer_ms(void) { return ticks * 10; }  /* ~10ms per tick */
