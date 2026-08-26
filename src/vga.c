/* corrOS — VGA text-mode driver (80×25, color 0x0F on 0x00). */
#include <stdint.h>
#include <string.h>

#define VGA_ADDR   0xB8000u
#define VGA_COLS   80
#define VGA_ROWS   25
#define VGA_ATTR   0x0F  /* white on black */

static volatile uint16_t *vga = (volatile uint16_t *)VGA_ADDR;
static int cursor_x = 0;
static int cursor_y = 0;

static void vga_scroll(void)
{
    for (int i = VGA_COLS; i < VGA_COLS * VGA_ROWS; i++)
        vga[i - VGA_COLS] = vga[i];
    for (int i = VGA_COLS * (VGA_ROWS - 1); i < VGA_COLS * VGA_ROWS; i++)
        vga[i] = (VGA_ATTR << 8) | ' ';
    cursor_y = VGA_ROWS - 1;
}

static void vga_putc(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        vga[cursor_y * VGA_COLS + cursor_x] = (VGA_ATTR << 8) | (uint8_t)c;
        cursor_x++;
    }
    if (cursor_x >= VGA_COLS) { cursor_x = 0; cursor_y++; }
    if (cursor_y >= VGA_ROWS) vga_scroll();
}

void vga_clear(void)
{
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        vga[i] = (VGA_ATTR << 8) | ' ';
    cursor_x = cursor_y = 0;
}

void vga_print(const char *s)
{
    while (*s) vga_putc(*s++);
}

void vga_print_color(const char *s, uint8_t fg, uint8_t bg)
{
    uint8_t attr = (bg << 4) | (fg & 0x0F);
    while (*s) {
        vga[cursor_y * VGA_COLS + cursor_x] = (attr << 8) | (uint8_t)*s;
        cursor_x++;
        if (*s == '\n') { cursor_x = 0; cursor_y++; }
        if (cursor_x >= VGA_COLS) { cursor_x = 0; cursor_y++; }
        if (cursor_y >= VGA_ROWS) vga_scroll();
        s++;
    }
}

void vga_newline(void)
{
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= VGA_ROWS) vga_scroll();
}
