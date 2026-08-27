/* croOS file_manager.c - GUI File Manager for HyperCorros */

#include "kernel/types.h"
#include "kernel/compositor.h"
#include "drivers/framebuffer.h"
#include "kernel/font.h"
#include "string.h"

static int fm_win = -1;
static int fm_scroll = 0;

static const char *fm_entries[] = {
    "[DIR] bin/",
    "[DIR] etc/",
    "[DIR] home/",
    "[DIR] tmp/",
    "[DIR] dev/",
    "[DIR] lib/",
    "[FILE] hello.cor",
    "[FILE] readme.txt",
    "[FILE] system.cfg",
    "[FILE] kernel.elf",
    "[FILE] logo.bmp",
    "[DIR] var/",
    "[FILE] config.ini",
    "[FILE] notes.txt",
    "[FILE] app.corpkg",
    "[FILE] game.corapp",
    "[FILE] archive.exe",
    "[FILE] package.deb",
    "[FILE] mobile.apk",
};
#define FM_ENTRY_COUNT 19

static void fm_on_draw(int id, int cx, int cy, int cw, int ch) {
    (void)id;
    fb_fill_rect(cx, cy, cw, ch, COL_WIN_BG);

    /* Header bar */
    fb_fill_rect(cx, cy, cw, 28, COL_TITLE_ACTIVE);
    fb_draw_string(cx + 12, cy + 6, "/ (root)", COL_TEXT_WHITE, COL_TITLE_ACTIVE, 1);

    /* Toolbar */
    int ty = cy + 28;
    fb_fill_rect(cx, ty, cw, 24, FB_RGB(35, 35, 48));
    fb_draw_string(cx + 8, ty + 4, "< Back  |  Up  |  Home  |  Refresh", COL_ACCENT, FB_RGB(35, 35, 48), 1);

    /* File list */
    int list_y = ty + 28;
    int line_h = FONT_HEIGHT + 4;
    int visible = (ch - 56 - 28) / line_h;

    for (int i = 0; i < visible && i + fm_scroll < FM_ENTRY_COUNT; i++) {
        int idx = i + fm_scroll;
        int row_y = list_y + i * line_h;

        /* Alternating row background */
        if (i % 2 == 0)
            fb_fill_rect(cx, row_y, cw, line_h, FB_RGB(25, 25, 36));

        /* Entry text */
        uint32_t text_col = COL_TEXT_WHITE;
        if (fm_entries[idx][1] == 'D')
            text_col = COL_ACCENT;
        else if (fm_entries[idx][0] == 'F')
            text_col = COL_TEXT_DIM;

        fb_draw_string(cx + 12, row_y + 2, fm_entries[idx], text_col, FB_RGB(25, 25, 36), 1);

        /* Size column */
        fb_draw_string(cx + cw - 80, row_y + 2,
                       idx % 3 == 0 ? "4.0 KB" : (idx % 3 == 1 ? "128 B" : "2.1 KB"),
                       COL_TEXT_DIM, FB_RGB(25, 25, 36), 1);
    }

    /* Status bar */
    int sb_y = cy + ch - 20;
    fb_fill_rect(cx, sb_y, cw, 20, COL_TASKBAR_BG);
    fb_draw_string(cx + 8, sb_y + 2,
                   "19 items  |  / (root filesystem)",
                   COL_TEXT_DIM, COL_TASKBAR_BG, 1);
}

static void fm_on_click(int id, int mx, int my) {
    (void)id;
    (void)mx;
    (void)my;
    /* Handle scroll and selection - TODO */
}

void app_file_manager_create(void) {
    if (fm_win >= 0 && comp.windows[fm_win].active) {
        gui_focus_window(fm_win);
        return;
    }
    int w = 560, h = 420;
    int x = ((int)fb.width - w) / 2 - 30;
    int y = ((int)fb.height - h) / 2 + 10;
    fm_win = gui_create_window(x, y, w, h, "File Manager");
    if (fm_win >= 0) {
        fm_scroll = 0;
        gui_set_draw_callback(fm_win, fm_on_draw);
        gui_set_click_callback(fm_win, fm_on_click);
        gui_set_key_callback(fm_win, 0);
    }
}
