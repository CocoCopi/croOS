/* croOS compositor.c - HyperCorros GUI Window Manager
 * Full pixel-based GUI: desktop wallpaper, window manager with
 * title bars and buttons, mouse cursor, taskbar with clock,
 * window dragging, and built-in apps. */

#include "kernel/types.h"
#include "kernel/compositor.h"
#include "drivers/framebuffer.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/timer.h"
#include "drivers/serial.h"
#include "kernel/font.h"
#include "mm/pmm.h"
#include "string.h"

compositor_t comp;

/* Expose fb for compositor */
extern framebuffer_t fb_global;
#define fb fb_global

/* ---- Cursor bitmap (16x16 arrow) ---- */
static const uint8_t cursor_data[16] = {
    0x00, 0x02, 0x36, 0x76, 0xF6, 0xFE, 0xFE, 0xFF,
    0x7F, 0x3E, 0x1E, 0x32, 0x22, 0x11, 0x00, 0x00
};

/* ---- Rounded rect helper ---- */
static int corner_inside(int px, int py, int cx, int cy, int r) {
    int dx = px - cx, dy = py - cy;
    return (dx*dx + dy*dy) <= r*r;
}

/* ============================================================
 *  DESKTOP
 * ============================================================ */

static void draw_desktop_bg(void) {
    /* Dark gradient background */
    fb_gradient_v(0, 0, fb.width, fb.height, FB_RGB(15, 15, 30), FB_RGB(8, 8, 15));
    /* Subtle grid pattern */
    for (int y = 0; y < (int)fb.height; y += 40)
        fb_draw_hline(0, y, fb.width, FB_RGB(20, 20, 35));
    for (int x = 0; x < (int)fb.width; x += 40)
        fb_draw_vline(x, 0, fb.height, FB_RGB(20, 20, 35));
    /* Desktop logo */
    const char *logo = "croOS";
    int logo_w = fb_text_width(logo, 4);
    int lx = ((int)fb.width - logo_w) / 2;
    int ly = ((int)fb.height - fb_char_height(4)) / 2 - 20;
    fb_draw_string(lx + 2, ly + 2, logo, FB_RGB(0, 0, 0), FB_RGB(0, 0, 0), 4);
    fb_draw_string(lx, ly, logo, FB_RGB(100, 140, 255), COL_DESKTOP_BG, 4);
    const char *sub = "HyperCorros Desktop Environment";
    int sw = fb_text_width(sub, 2);
    fb_draw_string(((int)fb.width - sw) / 2, ly + fb_char_height(4) + 16,
                   sub, COL_TEXT_DIM, COL_DESKTOP_BG, 2);
}

static void draw_desktop_icons(void) {
    int ix = 24, iy = 24, iw = 64, ih = 80, gap = 16;
    const char *icons[] = {"Terminal", "Files", "Editor", "Calc", "About"};
    const char *symbols[] = {">_", "[ ]", "TXT", "+/-", " ? "};
    uint32_t colors[] = {FB_RGB(60,100,200), FB_RGB(180,140,40), FB_RGB(100,170,100),
                         FB_RGB(180,80,60), FB_RGB(130,100,180)};
    for (int i = 0; i < 5; i++) {
        int x = ix + i * (iw + gap);
        fb_fill_rounded_rect(x, iy, iw, ih, 8, colors[i]);
        int sw = fb_text_width(symbols[i], 2);
        fb_draw_string(x + (iw - sw) / 2, iy + 12, symbols[i], COL_TEXT_WHITE, colors[i], 2);
        int lw = fb_text_width(icons[i], 1);
        fb_draw_string(x + (iw - lw) / 2, iy + ih + 4, icons[i], COL_TEXT_WHITE, COL_DESKTOP_BG, 1);
    }
}

/* ============================================================
 *  TASKBAR
 * ============================================================ */

static void draw_taskbar(void) {
    int ty = (int)fb.height - GUI_TASKBAR_H;
    fb_fill_rect(0, ty, fb.width, GUI_TASKBAR_H, COL_TASKBAR_BG);
    fb_draw_hline(0, ty, fb.width, FB_RGB(50, 50, 70));

    /* croOS button */
    int btn_x = 8, btn_y = ty + 4, btn_w = 72, btn_h = GUI_TASKBAR_H - 8;
    fb_fill_rounded_rect(btn_x, btn_y, btn_w, btn_h, 6, COL_ACCENT);
    int tw = fb_text_width("croOS", 1);
    fb_draw_string(btn_x + (btn_w - tw) / 2, btn_y + (btn_h - FONT_HEIGHT) / 2,
                   "croOS", COL_TEXT_WHITE, COL_ACCENT, 1);

    /* Running window buttons */
    int tx = btn_x + btn_w + 16;
    for (int i = 0; i < GUI_MAX_WINDOWS; i++) {
        if (!comp.windows[i].active || !comp.windows[i].visible) continue;
        int bw = fb_text_width(comp.windows[i].title, 1) + 24;
        uint32_t bg = comp.windows[i].focused ? COL_WIN_ACTIVE : COL_TASKBAR_BG;
        fb_fill_rounded_rect(tx, btn_y, bw, btn_h, 4, bg);
        fb_draw_string(tx + 12, btn_y + (btn_h - FONT_HEIGHT) / 2,
                       comp.windows[i].title, COL_TEXT_WHITE, bg, 1);
        tx += bw + 6;
    }

    /* Clock */
    uint32_t s = timer_get_seconds();
    uint32_t m = s / 60;
    uint32_t h = m / 60;
    char clock[16];
    clock[0] = '0' + (h / 10); clock[1] = '0' + (h % 10);
    clock[2] = ':'; clock[3] = '0' + (m % 60 / 10); clock[4] = '0' + (m % 10);
    clock[5] = ':'; clock[6] = '0' + (s % 60 / 10); clock[7] = '0' + (s % 10);
    clock[8] = '\0';
    int cw = fb_text_width(clock, 1);
    fb_draw_string((int)fb.width - cw - 16, ty + (GUI_TASKBAR_H - FONT_HEIGHT) / 2,
                   clock, COL_TASKBAR_TEXT, COL_TASKBAR_BG, 1);
}

/* ============================================================
 *  WINDOW MANAGEMENT
 * ============================================================ */

static void draw_title_bar(int id) {
    gui_window_t *w = &comp.windows[id];
    uint32_t bg = w->focused ? COL_TITLE_ACTIVE : COL_TITLE_INACT;
    fb_fill_rect(w->x, w->y, w->w, GUI_TITLEBAR_H, bg);

    fb_draw_string(w->x + 12, w->y + (GUI_TITLEBAR_H - FONT_HEIGHT) / 2,
                   w->title, COL_TEXT_WHITE, bg, 1);

    /* Window buttons */
    int btn_size = 14, btn_y = w->y + (GUI_TITLEBAR_H - btn_size) / 2, sp = 6;
    fb_fill_rounded_rect(w->x + w->w - 12 - btn_size, btn_y, btn_size, btn_size, 7, COL_BTN_CLOSE);
    fb_draw_string(w->x + w->w - 12 - btn_size + 4, btn_y + 1, "x", COL_TEXT_WHITE, COL_BTN_CLOSE, 1);
    fb_fill_rounded_rect(w->x + w->w - 12 - btn_size*2 - sp, btn_y, btn_size, btn_size, 7, COL_BTN_MIN);
    fb_draw_string(w->x + w->w - 12 - btn_size*2 - sp + 3, btn_y + 1, "_", COL_TEXT_WHITE, COL_BTN_MIN, 1);
    fb_fill_rounded_rect(w->x + w->w - 12 - btn_size*3 - sp*2, btn_y, btn_size, btn_size, 7, COL_BTN_MAX);
    fb_draw_string(w->x + w->w - 12 - btn_size*3 - sp*2 + 2, btn_y + 2, "[]", COL_TEXT_WHITE, COL_BTN_MAX, 1);
}

static void draw_window_content(int id) {
    gui_window_t *w = &comp.windows[id];
    if (w->minimized) return;
    int body_y = w->y + GUI_TITLEBAR_H;
    int body_h = w->h - GUI_TITLEBAR_H;
    fb_fill_rect(w->x + 1, body_y + 1, w->w - 2, body_h - 1, COL_WIN_BG);
    fb_draw_rect(w->x, w->y, w->w, w->h, COL_WIN_BORDER);
    if (w->on_draw) {
        w->on_draw(id, w->x + 2, body_y + 2, w->w - 4, body_h - 2);
    }
}

/* ============================================================
 *  MOUSE CURSOR
 * ============================================================ */

static void draw_cursor(void) {
    int mx = comp.mouse_x, my = comp.mouse_y;
    /* Arrow cursor */
    for (int i = 0; i < 12; i++) {
        fb_draw_vline(mx, my + i, 1, COL_TEXT_WHITE);
        fb_put_pixel(mx + i, my + i, COL_TEXT_WHITE);
    }
    fb_draw_hline(mx, my, 8, COL_TEXT_WHITE);
    fb_draw_vline(mx, my, 8, COL_TEXT_WHITE);
    for (int i = 0; i < 6; i++)
        fb_draw_vline(mx + 1 + i, my + 1 + i, 1, COL_TEXT_WHITE);
    fb_put_pixel(mx + 1, my + 1, COL_ACCENT);
}

/* ============================================================
 *  PUBLIC API
 * ============================================================ */

void compositor_init(void) {
    memset(&comp, 0, sizeof(comp));
    comp.active_window = -1;
    comp.mouse_x = fb.width / 2;
    comp.mouse_y = fb.height / 2;
    comp.running = 1;
    comp.drag_win = -1;
    serial_puts("[croOS] Compositor initialized\n");
}

int gui_create_window(int x, int y, int w, int h, const char *title) {
    for (int i = 0; i < GUI_MAX_WINDOWS; i++) {
        if (!comp.windows[i].active) {
            gui_window_t *win = &comp.windows[i];
            win->x = x; win->y = y; win->w = w; win->h = h;
            win->active = 1; win->visible = 1; win->minimized = 0;
            win->focused = 0; win->dragging = 0;
            win->on_draw = 0; win->on_key = 0; win->on_click = 0;
            memset(win->title, 0, GUI_MAX_TITLE);
            strncpy(win->title, title, GUI_MAX_TITLE - 1);
            memset(win->win_data, 0, sizeof(win->win_data));
            comp.window_count++;
            gui_focus_window(i);
            return i;
        }
    }
    return -1;
}

void gui_destroy_window(int id) {
    if (id < 0 || id >= GUI_MAX_WINDOWS) return;
    comp.windows[id].active = 0;
    comp.windows[id].visible = 0;
    comp.window_count--;
    if (comp.active_window == id) comp.active_window = -1;
}

void gui_set_window_title(int id, const char *title) {
    if (id < 0 || id >= GUI_MAX_WINDOWS) return;
    strncpy(comp.windows[id].title, title, GUI_MAX_TITLE - 1);
}

void gui_move_window(int id, int x, int y) {
    if (id < 0 || id >= GUI_MAX_WINDOWS) return;
    comp.windows[id].x = x; comp.windows[id].y = y;
}

void gui_resize_window(int id, int w, int h) {
    if (id < 0 || id >= GUI_MAX_WINDOWS) return;
    comp.windows[id].w = w; comp.windows[id].h = h;
}

void gui_focus_window(int id) {
    if (id < 0 || id >= GUI_MAX_WINDOWS) return;
    for (int i = 0; i < GUI_MAX_WINDOWS; i++) comp.windows[i].focused = 0;
    comp.windows[id].focused = 1;
    comp.active_window = id;
}

void gui_minimize_window(int id) {
    if (id < 0 || id >= GUI_MAX_WINDOWS) return;
    comp.windows[id].minimized = !comp.windows[id].minimized;
}

void gui_set_draw_callback(int id, void (*cb)(int,int,int,int,int)) {
    if (id >= 0 && id < GUI_MAX_WINDOWS) comp.windows[id].on_draw = cb;
}
void gui_set_key_callback(int id, void (*cb)(int,char)) {
    if (id >= 0 && id < GUI_MAX_WINDOWS) comp.windows[id].on_key = cb;
}
void gui_set_click_callback(int id, void (*cb)(int,int,int)) {
    if (id >= 0 && id < GUI_MAX_WINDOWS) comp.windows[id].on_click = cb;
}

int gui_get_window_data(int id, int idx) {
    if (id >= 0 && id < GUI_MAX_WINDOWS && idx >= 0 && idx < 8)
        return comp.windows[id].win_data[idx];
    return 0;
}
void gui_set_window_data(int id, int idx, int val) {
    if (id >= 0 && id < GUI_MAX_WINDOWS && idx >= 0 && idx < 8)
        comp.windows[id].win_data[idx] = val;
}

/* ============================================================
 *  DRAWING
 * ============================================================ */

void gui_draw_desktop(void) {
    draw_desktop_bg();
    draw_desktop_icons();
}
void gui_draw_taskbar(void) { draw_taskbar(); }
void gui_draw_window(int id) {
    if (id < 0 || id >= GUI_MAX_WINDOWS) return;
    if (!comp.windows[id].active || !comp.windows[id].visible) return;
    draw_title_bar(id);
    draw_window_content(id);
}
void gui_draw_mouse(void) { draw_cursor(); }

void gui_redraw_all(void) {
    gui_draw_desktop();
    for (int i = 0; i < GUI_MAX_WINDOWS; i++) gui_draw_window(i);
    gui_draw_taskbar();
    gui_draw_mouse();
}

/* ============================================================
 *  INPUT HANDLING
 * ============================================================ */

static int hit_test_button(int win_id, int mx, int my) {
    gui_window_t *w = &comp.windows[win_id];
    int bs = 14, by = w->y + (GUI_TITLEBAR_H - bs) / 2, sp = 6;
    if (mx >= w->x + w->w - 12 - bs && mx < w->x + w->w - 12 && my >= by && my < by + bs)
        return BTN_CLOSE;
    if (mx >= w->x + w->w - 12 - bs*2 - sp && mx < w->x + w->w - 12 - bs - sp && my >= by && my < by + bs)
        return BTN_MINIMIZE;
    if (mx >= w->x + w->w - 12 - bs*3 - sp*2 && mx < w->x + w->w - 12 - bs*2 - sp*2 && my >= by && my < by + bs)
        return BTN_MAXIMIZE;
    return -1;
}

static void handle_mouse(void) {
    mouse_state_t ms = mouse_get_state();
    comp.mouse_x += ms.dx * 2;
    comp.mouse_y += ms.dy * 2;
    if (comp.mouse_x < 0) comp.mouse_x = 0;
    if (comp.mouse_y < 0) comp.mouse_y = 0;
    if (comp.mouse_x >= (int)fb.width) comp.mouse_x = fb.width - 1;
    if (comp.mouse_y >= (int)fb.height) comp.mouse_y = fb.height - 1;

    uint8_t new_left = ms.buttons & 1;

    /* Window dragging */
    if (comp.drag_win >= 0 && comp.windows[comp.drag_win].dragging) {
        gui_window_t *w = &comp.windows[comp.drag_win];
        w->x = comp.mouse_x - w->drag_offset_x;
        w->y = comp.mouse_y - w->drag_offset_y;
        if (w->x < 0) w->x = 0;
        if (w->y < 0) w->y = 0;
        if (w->y + w->h > (int)fb.height - GUI_TASKBAR_H)
            w->y = fb.height - GUI_TASKBAR_H - w->h;
    }

    /* Left click down */
    if (new_left && !comp.mouse_left) {
        /* Taskbar button */
        int ty = fb.height - GUI_TASKBAR_H;
        if (comp.mouse_x >= 8 && comp.mouse_x < 80 &&
            comp.mouse_y >= ty + 4 && comp.mouse_y < ty + 4 + GUI_TASKBAR_H - 8) {
            app_about_create();
            comp.mouse_left = new_left;
            return;
        }

        /* Windows (front to back) */
        for (int i = GUI_MAX_WINDOWS - 1; i >= 0; i--) {
            gui_window_t *w = &comp.windows[i];
            if (!w->active || !w->visible || w->minimized) continue;
            if (comp.mouse_x < w->x || comp.mouse_x >= w->x + w->w ||
                comp.mouse_y < w->y || comp.mouse_y >= w->y + w->h) continue;

            int btn = hit_test_button(i, comp.mouse_x, comp.mouse_y);
            if (btn == BTN_CLOSE) { gui_destroy_window(i); comp.mouse_left = new_left; return; }
            if (btn == BTN_MINIMIZE) { gui_minimize_window(i); comp.mouse_left = new_left; return; }
            if (btn == BTN_MAXIMIZE) {
                if (w->w == (int)fb.width) {
                    w->x = w->win_data[0]; w->y = w->win_data[1];
                    w->w = w->win_data[2]; w->h = w->win_data[3];
                } else {
                    w->win_data[0] = w->x; w->win_data[1] = w->y;
                    w->win_data[2] = w->w; w->win_data[3] = w->h;
                    w->x = 0; w->y = 0;
                    w->w = fb.width; w->h = fb.height - GUI_TASKBAR_H;
                }
                comp.mouse_left = new_left;
                return;
            }
            if (comp.mouse_y < w->y + GUI_TITLEBAR_H) {
                w->dragging = 1;
                w->drag_offset_x = comp.mouse_x - w->x;
                w->drag_offset_y = comp.mouse_y - w->y;
                comp.drag_win = i;
                gui_focus_window(i);
                comp.mouse_left = new_left;
                return;
            }
            if (w->on_click)
                w->on_click(i, comp.mouse_x - w->x - 2, comp.mouse_y - w->y - GUI_TITLEBAR_H - 2);
            gui_focus_window(i);
            comp.mouse_left = new_left;
            return;
        }

        /* Desktop icons */
        int ix = 24, iy = 24, iw = 64, ih = 80, gap = 16;
        for (int i = 0; i < 5; i++) {
            int x = ix + i * (iw + gap);
            if (comp.mouse_x >= x && comp.mouse_x < x + iw &&
                comp.mouse_y >= iy && comp.mouse_y < iy + ih + 20) {
                switch (i) {
                    case 0: app_terminal_create(); break;
                    case 1: app_file_manager_create(); break;
                    case 2: app_text_editor_create(); break;
                    case 3: app_calculator_create(); break;
                    case 4: app_about_create(); break;
                }
                comp.mouse_left = new_left;
                return;
            }
        }
    }

    /* Left release */
    if (!new_left && comp.mouse_left) {
        if (comp.drag_win >= 0)
            comp.windows[comp.drag_win].dragging = 0;
        comp.drag_win = -1;
    }
    comp.mouse_left = new_left;
}

static void handle_keyboard(void) {
    if (comp.active_window < 0) return;
    gui_window_t *w = &comp.windows[comp.active_window];
    if (!w->active || w->minimized || !w->on_key) return;
    /* Poll all pending keypresses (non-blocking check) */
    while (kb_head != kb_tail) {
        char c = (char)kb_buffer[kb_tail];
        kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
        w->on_key(comp.active_window, c);
    }
}

/* ============================================================
 *  MAIN LOOP
 * ============================================================ */

void compositor_run(void) {
    gui_draw_desktop();
    gui_draw_taskbar();
    gui_draw_mouse();

    uint32_t last_tick = timer_get_ticks();
    while (comp.running) {
        handle_mouse();
        handle_keyboard();
        if (timer_get_ticks() - last_tick > 5) {
            gui_redraw_all();
            last_tick = timer_get_ticks();
        }
        asm volatile("hlt");
    }
}
