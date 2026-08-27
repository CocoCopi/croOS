/* croOS window.h - VGA text-mode window manager */
#ifndef _WINDOW_H
#define _WINDOW_H

#include "types.h"
#include "drivers/vga.h"

#define WIN_MAX_WINDOWS 16
#define WIN_MAX_TITLE   32

typedef struct {
    int x, y;
    int width, height;
    char title[WIN_MAX_TITLE];
    uint8_t title_color;
    uint8_t border_color;
    uint8_t bg_color;
    uint8_t active;
    uint8_t visible;
    uint16_t *backbuffer;  /* stored screen contents under window */
} window_t;

void    window_init(void);
int     window_create(int x, int y, int w, int h, const char *title, uint8_t color);
void    window_destroy(int id);
void    window_draw(int id);
void    window_activate(int id);
void    window_set_title(int id, const char *title);
void    window_move(int id, int x, int y);
void    window_resize(int id, int w, int h);
void    window_draw_border(int id);
void    window_clear(int id);
void    window_putchar(int id, int row, int col, char c, uint8_t color);
void    window_puts(int id, int row, int col, const char *str, uint8_t color);
void    window_fill(int id, char ch, uint8_t color);
void    window_draw_title_bar(int id);
void    window_redraw_all(void);
int     window_get_active(void);
int     window_count(void);

#endif
