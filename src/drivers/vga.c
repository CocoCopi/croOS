/* croOS vga.c — VGA text-mode driver
 * Directly writes to VGA memory at 0xB8000 for 80x25 text output.
 * Supports scrolling, cursor control, colored text, and drawing. */

#include "kernel/types.h"
#include "drivers/vga.h"

static uint16_t *vga_buffer = (uint16_t*)VGA_ADDR;
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t current_color = 0x0F;

static inline uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static inline uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_init(void) {
    vga_buffer = (uint16_t*)VGA_ADDR;
    current_color = make_color(VGA_WHITE, VGA_BLACK);
    vga_enable_cursor();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = make_entry(' ', current_color);
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_cursor(0, 0);
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    current_color = make_color(fg, bg);
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 8) & ~7;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            vga_buffer[cursor_row * VGA_WIDTH + cursor_col] = make_entry(' ', current_color);
        }
    } else {
        vga_buffer[cursor_row * VGA_WIDTH + cursor_col] = make_entry(c, current_color);
        cursor_col++;
    }

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
    if (cursor_row >= VGA_HEIGHT) {
        vga_scroll();
        cursor_row = VGA_HEIGHT - 1;
    }
    vga_cursor(cursor_row, cursor_col);
}

void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_put_dec(uint32_t n) {
    if (n == 0) { vga_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) vga_putchar(buf[--i]);
}

void vga_put_hex(uint32_t n) {
    vga_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (n >> i) & 0xF;
        vga_putchar(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    }
}

void vga_put_dec_signed(int32_t n) {
    if (n < 0) { vga_putchar('-'); vga_put_dec((uint32_t)-n); }
    else { vga_put_dec((uint32_t)n); }
}

void vga_scroll(void) {
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = make_entry(' ', current_color);
    }
}

void vga_cursor(int row, int col) {
    uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);
    outb(0x3D4, 14);
    outb(0x3D5, (pos >> 8) & 0xFF);
    outb(0x3D4, 15);
    outb(0x3D5, pos & 0xFF);
}

void vga_enable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_put_at(int row, int col, char c, uint8_t color) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
        vga_buffer[row * VGA_WIDTH + col] = make_entry(c, color);
    }
}

void vga_fill_rect(int r, int c, int w, int h, char ch, uint8_t color) {
    for (int y = r; y < r + h && y < VGA_HEIGHT; y++) {
        for (int x = c; x < c + w && x < VGA_WIDTH; x++) {
            vga_put_at(y, x, ch, color);
        }
    }
}
