/* croOS about.c - About/System Info window for HyperCorros */

#include "kernel/types.h"
#include "kernel/compositor.h"
#include "drivers/framebuffer.h"
#include "mm/pmm.h"
#include "kernel/font.h"
#include "string.h"

static int about_win = -1;

static void about_on_draw(int id, int cx, int cy, int cw, int ch) {
    (void)id;
    fb_fill_rect(cx, cy, cw, ch, COL_WIN_BG);

    /* Header gradient */
    fb_gradient_h(cx, cy, cw, 80, FB_RGB(30, 40, 80), FB_RGB(50, 20, 60));

    /* Logo */
    const char *logo = "croOS";
    fb_draw_string(cx + (cw - fb_text_width(logo, 3)) / 2, cy + 12,
                   logo, COL_TEXT_WHITE, FB_RGB(30, 40, 80), 3);

    const char *sub = "HyperCorros Desktop Environment";
    fb_draw_string(cx + (cw - fb_text_width(sub, 1)) / 2, cy + 56,
                   sub, COL_TEXT_DIM, FB_RGB(30, 40, 80), 1);

    /* Info section */
    int iy = cy + 96;
    int line_h = FONT_HEIGHT + 8;
    uint32_t label_col = COL_TEXT_DIM;
    uint32_t value_col = COL_TEXT_WHITE;

    const char *info[][2] = {
        {"Version:", "4.0.0"},
        {"Kernel:", "croOS microkernel"},
        {"Desktop:", "HyperCorros WM"},
        {"Arch:", "i686 (32-bit x86)"},
        {"Display:", "VESA Linear FB"},
        {"Shell:", "Kernel shell + GUI"},
        {"License:", "MIT License"},
        {"Build:", "Corros -> C -> GCC"},
        {"Author:", "CocoCopi"},
        {"Website:", "github.com/CocoCopi"},
    };
    int info_count = 10;

    for (int i = 0; i < info_count; i++) {
        int y = iy + i * line_h;
        if (y + FONT_HEIGHT > cy + ch) break;

        /* Alternating row */
        if (i % 2 == 0)
            fb_fill_rect(cx + 8, y - 2, cw - 16, line_h, FB_RGB(35, 35, 48));

        fb_draw_string(cx + 16, y, info[i][0], label_col, COL_WIN_BG, 1);
        fb_draw_string(cx + 120, y, info[i][1], value_col, COL_WIN_BG, 1);
    }

    /* Memory info */
    int my = iy + info_count * line_h + 8;
    fb_fill_rect(cx + 8, my, cw - 16, 1, FB_RGB(50, 50, 65));
    my += 8;
    fb_draw_string(cx + 16, my, "Memory:", COL_TEXT_DIM, COL_WIN_BG, 1);

    uint32_t free_kb = pmm_get_free_pages() * 4;
    uint32_t total_kb = pmm_get_total_pages() * 4;
    /* Simple percent bar */
    int bar_x = cx + 120, bar_y = my - 2, bar_w = 200, bar_h = 14;
    fb_fill_rounded_rect(bar_x, bar_y, bar_w, bar_h, 4, FB_RGB(40, 40, 55));
    uint32_t used_kb = total_kb - free_kb;
    int fill = (total_kb > 0) ? (used_kb * bar_w / total_kb) : 0;
    if (fill > 0)
        fb_fill_rounded_rect(bar_x, bar_y, fill, bar_h, 4, COL_ACCENT);

    /* Footer */
    fb_fill_rect(cx, cy + ch - 28, cw, 28, COL_TASKBAR_BG);
    fb_draw_string(cx + 12, cy + ch - 22, "Built from scratch with passion",
                   COL_TEXT_DIM, COL_TASKBAR_BG, 1);
}

void app_about_create(void) {
    if (about_win >= 0 && comp.windows[about_win].active) {
        gui_focus_window(about_win);
        return;
    }
    int w = 440, h = 420;
    int x = ((int)fb.width - w) / 2;
    int y = ((int)fb.height - h) / 2 - 20;
    about_win = gui_create_window(x, y, w, h, "About croOS");
    if (about_win >= 0) {
        gui_set_draw_callback(about_win, about_on_draw);
        gui_set_key_callback(about_win, 0);
        gui_set_click_callback(about_win, 0);
    }
}
