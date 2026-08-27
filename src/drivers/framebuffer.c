/* croOS framebuffer.c - Linear framebuffer driver
 * Pixel-level drawing via Multiboot-provided VESA linear framebuffer.
 * Implements all drawing primitives for the GUI compositor. */

#include "kernel/types.h"
#include "drivers/framebuffer.h"
#include "kernel/font.h"

framebuffer_t fb_global;
#define fb fb_global
static int fb_ready = 0;

void fb_init(framebuffer_t *info) {
    fb_global = *info;
    fb_ready = (fb.address != 0 && fb.width > 0 && fb.height > 0);
    if (fb_ready) {
        fb_clear(FB_COLOR_DARK_BG);
    }
}

uint8_t fb_available(void) { return fb_ready; }

/* ---- Low-level ---- */

void fb_put_pixel(int x, int y, uint32_t color) {
    if (!fb_ready) return;
    if (x < 0 || x >= (int)fb.width || y < 0 || y >= (int)fb.height) return;
    fb.address[y * (fb.pitch / 4) + x] = color;
}

uint32_t fb_get_pixel(int x, int y) {
    if (!fb_ready || x < 0 || x >= (int)fb.width || y < 0 || y >= (int)fb.height)
        return 0;
    return fb.address[y * (fb.pitch / 4) + x];
}

/* ---- Filled rectangles ---- */

void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (!fb_ready) return;
    for (int j = y; j < y + h; j++) {
        if (j < 0 || j >= (int)fb.height) continue;
        for (int i = x; i < x + w; i++) {
            if (i < 0 || i >= (int)fb.width) continue;
            fb.address[j * (fb.pitch / 4) + i] = color;
        }
    }
}

/* ---- Outlined rectangles ---- */

void fb_draw_rect(int x, int y, int w, int h, uint32_t color) {
    fb_draw_hline(x, y, w, color);
    fb_draw_hline(x, y + h - 1, w, color);
    fb_draw_vline(x, y, h, color);
    fb_draw_vline(x + w - 1, y, h, color);
}

void fb_draw_hline(int x, int y, int len, uint32_t color) {
    for (int i = 0; i < len; i++) fb_put_pixel(x + i, y, color);
}

void fb_draw_vline(int x, int y, int len, uint32_t color) {
    for (int i = 0; i < len; i++) fb_put_pixel(x, y + i, color);
}

/* ---- Lines (Bresenham) ---- */

void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        fb_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* ---- Circles (Midpoint) ---- */

void fb_draw_circle(int cx, int cy, int r, uint32_t color) {
    int x = r, y = 0, d = 1 - r;
    while (x >= y) {
        fb_put_pixel(cx+x, cy+y, color);
        fb_put_pixel(cx-x, cy+y, color);
        fb_put_pixel(cx+x, cy-y, color);
        fb_put_pixel(cx-x, cy-y, color);
        fb_put_pixel(cx+y, cy+x, color);
        fb_put_pixel(cx-y, cy+x, color);
        fb_put_pixel(cx+y, cy-x, color);
        fb_put_pixel(cx-y, cy-x, color);
        y++;
        if (d <= 0) { d += 2*y + 1; }
        else { x--; d += 2*(y - x) + 1; }
    }
}

void fb_fill_circle(int cx, int cy, int r, uint32_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r)
                fb_put_pixel(cx + x, cy + y, color);
        }
    }
}

/* ---- Clear ---- */

void fb_clear(uint32_t color) {
    if (!fb_ready) return;
    uint32_t pixels = fb.width * fb.height;
    for (uint32_t i = 0; i < pixels; i++)
        fb.address[i] = color;
}

/* ---- Gradients ---- */

void fb_gradient_v(int x, int y, int w, int h, uint32_t c_top, uint32_t c_bot) {
    for (int j = 0; j < h; j++) {
        uint8_t t = (uint8_t)(j * 255 / (h > 0 ? h : 1));
        uint8_t r = FB_RED(c_top)   + (int)(FB_RED(c_bot)   - FB_RED(c_top))   * t / 255;
        uint8_t g = FB_GREEN(c_top) + (int)(FB_GREEN(c_bot) - FB_GREEN(c_top)) * t / 255;
        uint8_t b = FB_BLUE(c_top)  + (int)(FB_BLUE(c_bot)  - FB_BLUE(c_top))  * t / 255;
        fb_draw_hline(x, y + j, w, FB_RGB(r, g, b));
    }
}

void fb_gradient_h(int x, int y, int w, int h, uint32_t c_left, uint32_t c_right) {
    for (int i = 0; i < w; i++) {
        uint8_t t = (uint8_t)(i * 255 / (w > 0 ? w : 1));
        uint8_t r = FB_RED(c_left)   + (int)(FB_RED(c_right)   - FB_RED(c_left))   * t / 255;
        uint8_t g = FB_GREEN(c_left) + (int)(FB_GREEN(c_right) - FB_GREEN(c_left)) * t / 255;
        uint8_t b = FB_BLUE(c_left)  + (int)(FB_BLUE(c_right)  - FB_BLUE(c_left))  * t / 255;
        fb_draw_vline(x + i, y, h, FB_RGB(r, g, b));
    }
}

/* ---- Blit raw pixel data ---- */

void fb_blit(int x, int y, int w, int h, uint32_t *data) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            fb_put_pixel(x + i, y + j, data[j * w + i]);
        }
    }
}

/* ---- Alpha blending ---- */

uint32_t fb_blend(uint32_t fg, uint32_t bg, uint8_t alpha) {
    uint8_t a = alpha;
    uint8_t inv = 255 - a;
    uint8_t r = (FB_RED(fg) * a + FB_RED(bg) * inv) / 255;
    uint8_t g = (FB_GREEN(fg) * a + FB_GREEN(bg) * inv) / 255;
    uint8_t b = (FB_BLUE(fg) * a + FB_BLUE(bg) * inv) / 255;
    return FB_RGB(r, g, b);
}

/* ---- Rounded rectangles ---- */

static int corner_dist(int x, int y, int cx, int cy, int r) {
    int dx = x - cx, dy = y - cy;
    return dx*dx + dy*dy;
}

void fb_fill_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            int draw = 0;
            /* Check corners */
            if (i < x + r && j < y + r) {
                draw = (corner_dist(i, j, x + r, y + r, r) <= r*r) ? 1 : 0;
            } else if (i >= x + w - r && j < y + r) {
                draw = (corner_dist(i, j, x + w - r - 1, y + r, r) <= r*r) ? 1 : 0;
            } else if (i < x + r && j >= y + h - r) {
                draw = (corner_dist(i, j, x + r, y + h - r - 1, r) <= r*r) ? 1 : 0;
            } else if (i >= x + w - r && j >= y + h - r) {
                draw = (corner_dist(i, j, x + w - r - 1, y + h - r - 1, r) <= r*r) ? 1 : 0;
            } else {
                draw = 1; /* Inside body */
            }
            if (draw) fb_put_pixel(i, j, color);
        }
    }
}

void fb_draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    fb_fill_rounded_rect(x, y, w, h, r, color);
    fb_fill_rounded_rect(x + 1, y + 1, w - 2, h - 2, r > 0 ? r - 1 : 0, FB_COLOR_DARK_BG);
}

/* ---- Shadow ---- */

void fb_draw_shadow(int x, int y, int w, int h, int radius, uint8_t alpha) {
    for (int j = -radius; j < h + radius; j++) {
        for (int i = -radius; i < w + radius; i++) {
            /* Distance to nearest edge */
            int dx = (i < 0) ? -i : (i >= w ? i - w + 1 : 0);
            int dy = (j < 0) ? -j : (j >= h ? j - h + 1 : 0);
            int dist = dx + dy;
            if (dist > 0 && dist <= radius) {
                uint8_t a = alpha * (radius - dist) / radius;
                if (a > 0) {
                    int px = x + i, py = y + j;
                    if (px >= 0 && px < (int)fb.width && py >= 0 && py < (int)fb.height) {
                        uint32_t bg = fb_get_pixel(px, py);
                        fb_put_pixel(px, py, fb_blend(FB_COLOR_BLACK, bg, a));
                    }
                }
            }
        }
    }
}

/* ---- Text rendering (bitmap font) ---- */

void fb_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg, int scale) {
    int ch = (unsigned char)c;
    if (ch >= FONT_NUM_GLYPHS) ch = 0;
    const uint8_t *glyph = font_get_glyph(ch);
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (bits & (0x80 >> col)) ? fg : bg;
            if (scale <= 1) {
                fb_put_pixel(x + col, y + row, color);
            } else {
                fb_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void fb_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg, int scale) {
    int cx = x;
    while (*str) {
        if (*str == '\n') { cx = x; y += FONT_HEIGHT * scale; str++; continue; }
        fb_draw_char(cx, y, *str, fg, bg, scale);
        cx += FONT_WIDTH * scale;
        str++;
    }
}

int fb_char_width(int scale) { return FONT_WIDTH * scale; }
int fb_char_height(int scale) { return FONT_HEIGHT * scale; }

int fb_text_width(const char *str, int scale) {
    int len = 0;
    while (*str++) len++;
    return len * FONT_WIDTH * scale;
}
