#ifndef CORROS_VGA_H
#define CORROS_VGA_H
#include <stdint.h>
void vga_clear(void);
void vga_print(const char *s);
void vga_print_color(const char *s, uint8_t fg, uint8_t bg);
void vga_newline(void);
void vga_putc(char c);  /* for speak_num */
#endif
