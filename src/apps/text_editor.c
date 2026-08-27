/* croOS text_editor.c - GUI Text Editor for HyperCorros */

#include "kernel/types.h"
#include "kernel/compositor.h"
#include "drivers/framebuffer.h"
#include "kernel/font.h"
#include "string.h"

static int te_win = -1;
static char te_buffer[4096];
static int te_len = 0;
static int te_cursor = 0;
static int te_scroll_y = 0;
static int te_cursor_blink = 1;

static void te_on_draw(int id, int cx, int cy, int cw, int ch) {
    (void)id;
    fb_fill_rect(cx, cy, cw, ch, COL_WIN_BG);

    /* Toolbar */
    fb_fill_rect(cx, cy, cw, 24, COL_TITLE_ACTIVE);
    fb_draw_string(cx + 8, cy + 4, "File  |  Edit  |  View", COL_TEXT_WHITE, COL_TITLE_ACTIVE, 1);
    fb_draw_string(cx + cw - 100, cy + 4, "hello.cor", COL_ACCENT, COL_TITLE_ACTIVE, 1);

    /* Line numbers */
    int text_y = cy + 28;
    int line_h = FONT_HEIGHT + 2;
    int visible = (ch - 28 - 24) / line_h;
    int text_x = cx + 40;

    fb_fill_rect(cx, text_y, 36, ch - 28 - 24, FB_RGB(20, 20, 30));

    /* Count lines up to cursor */
    int cur_line = 1;
    for (int i = 0; i < te_cursor && i < te_len; i++)
        if (te_buffer[i] == '\n') cur_line++;

    /* Render visible portion */
    int start_line = te_scroll_y;
    for (int i = 0; i < visible; i++) {
        int ln = start_line + i + 1;
        int ly = text_y + i * line_h;

        /* Line number */
        char lnum[8]; int li = 0;
        int tmp = ln;
        if (tmp == 0) lnum[li++] = '0';
        else { char t2[8]; int ti2 = 0; while (tmp) { t2[ti2++] = '0' + tmp % 10; tmp /= 10; }
        while (ti2) lnum[li++] = t2[--ti2]; }
        lnum[li] = '\0';
        fb_draw_string(cx + 4, ly, lnum, COL_TEXT_DIM, FB_RGB(20, 20, 30), 1);

        /* Find line content */
        int line_start = 0;
        for (int j = 0; j < ln - 1 && j < te_len; j++)
            if (te_buffer[j] == '\n') line_start = j + 1;
        int line_end = line_start;
        while (line_end < te_len && te_buffer[line_end] != '\n') line_end++;

        if (line_end > line_start) {
            /* Draw text */
            int max_chars = (cw - 48) / fb_char_width(1);
            int chars_to_draw = line_end - line_start;
            if (chars_to_draw > max_chars) chars_to_draw = max_chars;
            char tmp_buf[256];
            for (int j = 0; j < chars_to_draw; j++) tmp_buf[j] = te_buffer[line_start + j];
            tmp_buf[chars_to_draw] = '\0';
            fb_draw_string(text_x, ly, tmp_buf, COL_TEXT_WHITE, COL_WIN_BG, 1);
        }
    }

    /* Cursor */
    if (te_cursor_blink && te_win >= 0 && comp.windows[te_win].focused) {
        int cur_line_start = 0;
        int cl = 1;
        for (int i = 0; i < te_cursor && i < te_len; i++) {
            if (te_buffer[i] == '\n') { cl++; cur_line_start = i + 1; }
        }
        int cx_pos = text_x + (te_cursor - cur_line_start) * fb_char_width(1);
        int cy_pos = text_y + (cl - start_line - 1) * line_h;
        if (cy_pos >= text_y && cy_pos < cy + ch - 24) {
            fb_fill_rect(cx_pos, cy_pos, 2, FONT_HEIGHT, COL_ACCENT);
        }
    }
    te_cursor_blink = !te_cursor_blink;
}

static void te_on_key(int id, char key) {
    (void)id;
    if (key == '\b') {
        if (te_cursor > 0) {
            for (int i = te_cursor - 1; i < te_len - 1; i++)
                te_buffer[i] = te_buffer[i + 1];
            te_cursor--;
            te_len--;
        }
    } else if (key == '\n') {
        if (te_len < 4095) {
            for (int i = te_len; i > te_cursor; i--)
                te_buffer[i] = te_buffer[i - 1];
            te_buffer[te_cursor] = '\n';
            te_cursor++;
            te_len++;
        }
    } else {
        if (te_len < 4095) {
            for (int i = te_len; i > te_cursor; i--)
                te_buffer[i] = te_buffer[i - 1];
            te_buffer[te_cursor] = key;
            te_cursor++;
            te_len++;
        }
    }
    te_buffer[te_len] = '\0';

    /* Auto-scroll */
    int line_h = FONT_HEIGHT + 2;
    int visible = (comp.windows[te_win].h - 28 - 24) / line_h;
    int cur_line = 1;
    for (int i = 0; i < te_cursor && i < te_len; i++)
        if (te_buffer[i] == '\n') cur_line++;
    if (cur_line - te_scroll_y >= visible) te_scroll_y = cur_line - visible + 1;
    if (cur_line - te_scroll_y < 1) te_scroll_y = cur_line - 1;
}

void app_text_editor_create(void) {
    if (te_win >= 0 && comp.windows[te_win].active) {
        gui_focus_window(te_win);
        return;
    }
    int w = 640, h = 480;
    int x = ((int)fb.width - w) / 2 + 20;
    int y = ((int)fb.height - h) / 2 - 10;
    te_win = gui_create_window(x, y, w, h, "Text Editor - hello.cor");
    if (te_win >= 0) {
        /* Pre-populate with sample Corros code */
        const char *sample =
            "# Hello World in Corros\n"
            "greet \"World\"\n"
            "\n"
            "craft add(a, b):\n"
            "  yield a + b\n"
            "\n"
            "forge result = add(2, 3)\n"
            "greet(result)\n"
            "\n"
            "# croOS Application Framework\n"
            "app MyApp:\n"
            "  on start:\n"
            "    greet(\"App started!\")\n"
            "  on key(k):\n"
            "    greet(k)\n";
        te_len = 0;
        te_cursor = 0;
        while (*sample && te_len < 4095)
            te_buffer[te_len++] = *sample++;
        te_buffer[te_len] = '\0';
        te_scroll_y = 0;

        gui_set_draw_callback(te_win, te_on_draw);
        gui_set_key_callback(te_win, te_on_key);
        gui_set_click_callback(te_win, 0);
    }
}
