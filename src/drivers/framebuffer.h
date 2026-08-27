/* croOS framebuffer.h - Linear framebuffer for GUI
 * Pixel-level drawing via Multiboot-provided VESA framebuffer.
 * Provides primitives: pixel, line, rect, filled rect, text. */
#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include "kernel/types.h"

typedef struct {
    uint32_t *address;   /* Linear framebuffer base address */
    uint32_t  pitch;     /* Bytes per scanline */
    uint32_t  width;     /* Width in pixels */
    uint32_t  height;    /* Height in pixels */
    uint8_t   bpp;       /* Bits per pixel (32) */
    uint8_t   available; /* 1 if framebuffer was provided by bootloader */
} framebuffer_t;

/* 32-bit ARGB color helpers */
#define FB_RGB(r,g,b)   (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define FB_RGBA(r,g,b,a) (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define FB_RED(c)       (((c) >> 16) & 0xFF)
#define FB_GREEN(c)     (((c) >> 8) & 0xFF)
#define FB_BLUE(c)      ((c) & 0xFF)

/* Common colors */
#define FB_COLOR_BLACK    FB_RGB(0, 0, 0)
#define FB_COLOR_WHITE    FB_RGB(255, 255, 255)
#define FB_COLOR_RED      FB_RGB(220, 50, 47)
#define FB_COLOR_GREEN    FB_RGB(133, 153, 0)
#define FB_COLOR_BLUE     FB_RGB(38, 139, 210)
#define FB_COLOR_CYAN     FB_RGB(42, 161, 152)
#define FB_COLOR_YELLOW   FB_RGB(181, 137, 0)
#define FB_COLOR_MAGENTA  FB_RGB(211, 54, 130)
#define FB_COLOR_ORANGE   FB_RGB(203, 75, 22)
#define FB_COLOR_GRAY     FB_RGB(128, 128, 128)
#define FB_COLOR_DGRAY    FB_RGB(64, 64, 64)
#define FB_COLOR_LGRAY    FB_RGB(192, 192, 192)
#define FB_COLOR_DARK_BG  FB_RGB(20, 20, 30)
#define FB_COLOR_DESKTOP  FB_RGB(15, 15, 25)

/* Init */
void fb_init(framebuffer_t *fb);
uint8_t fb_available(void);

/* Drawing primitives */
void fb_put_pixel(int x, int y, uint32_t color);
uint32_t fb_get_pixel(int x, int y);
void fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_hline(int x, int y, int len, uint32_t color);
void fb_draw_vline(int x, int y, int len, uint32_t color);
void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void fb_draw_circle(int cx, int cy, int r, uint32_t color);
void fb_fill_circle(int cx, int cy, int r, uint32_t color);
void fb_clear(uint32_t color);
void fb_gradient_v(int x, int y, int w, int h, uint32_t c_top, uint32_t c_bot);
void fb_gradient_h(int x, int y, int w, int h, uint32_t c_left, uint32_t c_right);
void fb_blit(int x, int y, int w, int h, uint32_t *data);

/* Text (uses embedded bitmap font) */
void fb_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg, int scale);
void fb_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg, int scale);
int  fb_text_width(const char *str, int scale);
int  fb_char_width(int scale);
int  fb_char_height(int scale);

/* Rounded rectangle (for modern UI) */
void fb_fill_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void fb_draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);

/* Shadow effect */
void fb_draw_shadow(int x, int y, int w, int h, int radius, uint8_t alpha);

/* Blending */
uint32_t fb_blend(uint32_t fg, uint32_t bg, uint8_t alpha);

#endif
