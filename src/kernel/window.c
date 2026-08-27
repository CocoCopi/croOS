/* croOS window.c - Text-mode window manager
 * Draws bordered windows with title bars on VGA text mode.
 * Supports overlapping windows, activation, and backbuffer restore. */

#include "types.h"
#include "window.h"
#include "string.h"

static window_t windows[WIN_MAX_WINDOWS];
static int active_window = -1;

static inline uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

void window_init(void) {
    memset(windows, 0, sizeof(windows));
    active_window = -1;
}

int window_create(int x, int y, int w, int h, const char *title, uint8_t color) {
    for (int i = 0; i < WIN_MAX_WINDOWS; i++) {
        if (!windows[i].active) {
            windows[i].x = x;
            windows[i].y = y;
            windows[i].width = w;
            windows[i].height = h;
            windows[i].title_color = color;
            windows[i].border_color = make_color(VGA_WHITE, VGA_BLACK);
            windows[i].bg_color = make_color(VGA_WHITE, VGA_BLACK);
            windows[i].active = 1;
            windows[i].visible = 1;
            strncpy(windows[i].title, title, WIN_MAX_TITLE - 1);

            /* Save backbuffer */
            windows[i].backbuffer = (uint16_t*)0xB8000;  /* TODO: proper alloc */

            window_draw(i);
            return i;
        }
    }
    return -1;
}

void window_destroy(int id) {
    if (id < 0 || id >= WIN_MAX_WINDOWS || !windows[id].active) return;

    /* Restore backbuffer */
    if (windows[id].backbuffer) {
        for (int row = windows[id].y; row < windows[id].y + windows[id].height && row < VGA_HEIGHT; row++) {
            for (int col = windows[id].x; col < windows[id].x + windows[id].width && col < VGA_WIDTH; col++) {
                uint16_t *vga = (uint16_t*)VGA_ADDR;
                vga[row * VGA_WIDTH + col] = windows[id].backbuffer[(row - windows[id].y) * windows[id].width + (col - windows[id].x)];
            }
        }
    }

    windows[id].active = 0;
    windows[id].visible = 0;

    if (active_window == id) active_window = -1;
}

static void draw_char(int row, int col, char ch, uint8_t color) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
        uint16_t *vga = (uint16_t*)VGA_ADDR;
        vga[row * VGA_WIDTH + col] = (uint16_t)ch | ((uint16_t)color << 8);
    }
}

void window_draw_border(int id) {
    window_t *w = &windows[id];
    uint8_t border = w->active ? make_color(VGA_LCYAN, VGA_BLACK) : make_color(VGA_GRAY, VGA_BLACK);

    /* Top border */
    draw_char(w->y, w->x, '+', border);
    for (int i = 1; i < w->width - 1; i++)
        draw_char(w->y, w->x + i, '-', border);
    draw_char(w->y, w->x + w->width - 1, '+', border);

    /* Side borders */
    for (int row = 1; row < w->height - 1; row++) {
        draw_char(w->y + row, w->x, '|', border);
        for (int col = 1; col < w->width - 1; col++)
            draw_char(w->y + row, w->x + col, ' ', make_color(VGA_WHITE, VGA_BLACK));
        draw_char(w->y + row, w->x + w->width - 1, '|', border);
    }

    /* Bottom border */
    draw_char(w->y + w->height - 1, w->x, '+', border);
    for (int i = 1; i < w->width - 1; i++)
        draw_char(w->y + w->height - 1, w->x + i, '-', border);
    draw_char(w->y + w->height - 1, w->x + w->width - 1, '+', border);

    /* Title bar */
    window_draw_title_bar(id);
}

void window_draw_title_bar(int id) {
    window_t *w = &windows[id];
    uint8_t title_bg = w->active ? make_color(VGA_WHITE, VGA_BLUE) : make_color(VGA_WHITE, VGA_GRAY);

    /* Fill title area */
    for (int col = 1; col < w->width - 1; col++)
        draw_char(w->y, w->x + col, ' ', title_bg);

    /* Draw title text */
    int title_len = strlen(w->title);
    int start = 1 + ((w->width - 2 - title_len) / 2);
    for (int i = 0; i < title_len && start + i < w->width - 1; i++)
        draw_char(w->y, w->x + start + i, w->title[i], title_bg);
}

void window_draw(int id) {
    if (id < 0 || id >= WIN_MAX_WINDOWS || !windows[id].active) return;
    window_draw_border(id);
}

void window_activate(int id) {
    if (id >= 0 && id < WIN_MAX_WINDOWS && windows[id].active) {
        if (active_window >= 0 && active_window != windows[id].x)
            window_draw_border(active_window);
        active_window = id;
        window_draw_border(id);
    }
}

void window_set_title(int id, const char *title) {
    if (id < 0 || id >= WIN_MAX_WINDOWS || !windows[id].active) return;
    strncpy(windows[id].title, title, WIN_MAX_TITLE - 1);
    window_draw_title_bar(id);
}

void window_move(int id, int x, int y) {
    if (id < 0 || id >= WIN_MAX_WINDOWS || !windows[id].active) return;
    /* TODO: restore old area, redraw at new position */
    windows[id].x = x;
    windows[id].y = y;
    window_draw(id);
}

void window_resize(int id, int w, int h) {
    if (id < 0 || id >= WIN_MAX_WINDOWS || !windows[id].active) return;
    windows[id].width = w;
    windows[id].height = h;
    window_draw(id);
}

void window_clear(int id) {
    window_t *w = &windows[id];
    for (int row = 1; row < w->height - 1; row++)
        for (int col = 1; col < w->width - 1; col++)
            draw_char(w->y + row, w->x + col, ' ', w->bg_color);
}

void window_putchar(int id, int row, int col, char c, uint8_t color) {
    window_t *w = &windows[id];
    draw_char(w->y + 1 + row, w->x + 1 + col, c, color);
}

void window_puts(int id, int row, int col, const char *str, uint8_t color) {
    while (*str) {
        window_putchar(id, row, col++, *str++, color);
        if (col >= (int)windows[id].width - 2) { col = 0; row++; }
    }
}

void window_fill(int id, char ch, uint8_t color) {
    window_t *w = &windows[id];
    for (int row = 1; row < w->height - 1; row++)
        for (int col = 1; col < w->width - 1; col++)
            draw_char(w->y + row, w->x + col, ch, color);
}

void window_redraw_all(void) {
    for (int i = 0; i < WIN_MAX_WINDOWS; i++) {
        if (windows[i].active && windows[i].visible)
            window_draw(i);
    }
}

int window_get_active(void) { return active_window; }
int window_count(void) {
    int n = 0;
    for (int i = 0; i < WIN_MAX_WINDOWS; i++)
        if (windows[i].active) n++;
    return n;
}
