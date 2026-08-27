/* croOS vga.h — VGA text-mode driver (80x25, 16 colors) */
#ifndef _VGA_H
#define _VGA_H

#include "kernel/types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_ADDR   0xB8000

/* Colors */
#define VGA_BLACK   0x0
#define VGA_WHITE   0xF
#define VGA_RED     0x4
#define VGA_GREEN   0x2
#define VGA_BLUE    0x1
#define VGA_CYAN    0x3
#define VGA_YELLOW  0xE
#define VGA_MAGENTA 0x5
#define VGA_BROWN   0x6
#define VGA_GRAY    0x7
#define VGA_LGREEN  0xA
#define VGA_LRED    0xC
#define VGA_LBLUE   0x9
#define VGA_LCYAN   0xB
#define VGA_LYELLOW 0xD

void vga_init(void);
void vga_clear(void);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_putchar(char c);
void vga_puts(const char *str);
void vga_put_dec(uint32_t n);
void vga_put_hex(uint32_t n);
void vga_put_dec_signed(int32_t n);
void vga_scroll(void);
void vga_cursor(int row, int col);
void vga_enable_cursor(void);
void vga_disable_cursor(void);
void vga_put_at(int row, int col, char c, uint8_t color);
void vga_fill_rect(int r, int c, int w, int h, char ch, uint8_t color);

#endif
