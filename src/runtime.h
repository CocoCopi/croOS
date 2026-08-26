/* corrOS — freestanding runtime declarations.
 * The Corros C backend emits #include <stdio.h>, <stdlib.h>, <string.h>,
 * <math.h>, <time.h>.  We intercept those with our own headers (via
 * -Iinclude -nostdinc) and provide equivalent implementations here.
 */
#ifndef CORROS_RUNTIME_H
#define CORROS_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

/* VGA console (vga.c) */
void vga_clear(void);
void vga_print(const char *s);
void vga_print_color(const char *s, uint8_t fg, uint8_t bg);
void vga_newline(void);

/* Timer (timer.c) — returns ms since boot */
unsigned int timer_ms(void);

/* Port I/O (ports.c) */
uint8_t  inb(uint16_t port);
void     outb(uint16_t port, uint8_t val);

/* Keyboard (keyboard.c) — returns scancode or -1 */
int kb_read_scancode(void);

#endif /* CORROS_RUNTIME_H */
