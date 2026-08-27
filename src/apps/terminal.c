/* croOS terminal.c - Built-in terminal emulator for GUI
 * Full terminal with command input, output display, scrolling. */

#include "kernel/types.h"
#include "kernel/compositor.h"
#include "drivers/framebuffer.h"
#include "drivers/timer.h"
#include "mm/pmm.h"
#include "kernel/font.h"
#include "string.h"

#define TERM_MAX_LINES 256
#define TERM_LINE_LEN  128
#define TERM_INPUT_LEN 128

static char term_lines[TERM_MAX_LINES][TERM_LINE_LEN];
static int term_line_count = 0;
static int term_scroll = 0;
static char term_input[TERM_INPUT_LEN];
static int term_input_len = 0;
static int term_cursor_blink = 1;
static int term_win = -1;

static void term_print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            term_line_count++;
            if (term_line_count > TERM_MAX_LINES) term_line_count = TERM_MAX_LINES;
            int last = term_line_count - 1;
            memset(term_lines[last], 0, TERM_LINE_LEN);
            str++;
            continue;
        }
        int last = term_line_count > 0 ? term_line_count - 1 : 0;
        int len = strlen(term_lines[last]);
        if (len < TERM_LINE_LEN - 1) {
            term_lines[last][len] = *str;
            term_lines[last][len + 1] = '\0';
        }
        str++;
    }
    if (term_line_count == 0) term_line_count = 1;
}

static void term_execute(const char *cmd) {
    /* Echo command */
    term_print("$ ");
    term_print(cmd);
    term_print("\n");

    if (strcmp(cmd, "help") == 0) {
        term_print("Available commands:\n");
        term_print("  help     - show this help\n");
        term_print("  clear    - clear terminal\n");
        term_print("  echo     - print text\n");
        term_print("  version  - croOS version\n");
        term_print("  mem      - memory info\n");
        term_print("  uptime   - system uptime\n");
        term_print("  ls       - list files\n");
        term_print("  whoami   - current user\n");
        term_print("  date     - current date\n");
        term_print("  uname    - system info\n");
        term_print("  calc     - calculator mode\n");
    } else if (strcmp(cmd, "clear") == 0) {
        term_line_count = 0;
        memset(term_lines, 0, sizeof(term_lines));
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        term_print(cmd + 5);
        term_print("\n");
    } else if (strcmp(cmd, "version") == 0) {
        term_print("croOS 4.0.0 (32-bit)\n");
        term_print("HyperCorros Desktop Environment\n");
    } else if (strcmp(cmd, "mem") == 0) {
        uint32_t free = pmm_get_free_pages() * 4;
        uint32_t total = pmm_get_total_pages() * 4;
        term_print("Memory: ");
        /* Simple itoa */
        char buf[16]; int bi = 0;
        uint32_t n = free;
        if (n == 0) buf[bi++] = '0';
        else { char tmp[12]; int ti = 0; while (n) { tmp[ti++] = '0' + n % 10; n /= 10; }
        while (ti) buf[bi++] = tmp[--ti]; }
        buf[bi] = 'K'; buf[bi+1] = 'B'; buf[bi+2] = '/'; bi += 3;
        n = total;
        char tmp2[12]; int ti2 = 0;
        if (n == 0) tmp2[ti2++] = '0';
        else { while (n) { tmp2[ti2++] = '0' + n % 10; n /= 10; } }
        while (ti2) buf[bi++] = tmp2[--ti2];
        buf[bi++] = 'K'; buf[bi++] = 'B'; buf[bi] = '\0';
        term_print(buf);
        term_print("\n");
    } else if (strcmp(cmd, "uptime") == 0) {
        uint32_t s = timer_get_seconds();
        uint32_t m = s / 60;
        uint32_t h = m / 60;
        term_print("Uptime: ");
        char buf[16]; int bi = 0;
        buf[bi++] = '0' + (h / 10); buf[bi++] = '0' + (h % 10);
        buf[bi++] = 'h';
        buf[bi++] = '0' + ((m % 60) / 10); buf[bi++] = '0' + (m % 10);
        buf[bi++] = 'm';
        buf[bi++] = '0' + ((s % 60) / 10); buf[bi++] = '0' + (s % 10);
        buf[bi++] = 's'; buf[bi] = '\0';
        term_print(buf);
        term_print("\n");
    } else if (strcmp(cmd, "whoami") == 0) {
        term_print("root@croOS\n");
    } else if (strcmp(cmd, "date") == 0) {
        uint32_t s = timer_get_seconds();
        uint32_t m = s / 60;
        uint32_t h = m / 60;
        term_print("System time: ");
        char buf[16]; int bi = 0;
        buf[bi++] = '0' + (h / 10); buf[bi++] = '0' + (h % 10);
        buf[bi++] = ':';
        buf[bi++] = '0' + ((m % 60) / 10); buf[bi++] = '0' + (m % 10);
        buf[bi++] = ':';
        buf[bi++] = '0' + ((s % 60) / 10); buf[bi++] = '0' + (s % 10);
        buf[bi] = '\0';
        term_print(buf);
        term_print("\n");
    } else if (strcmp(cmd, "uname") == 0) {
        term_print("croOS 4.0.0 HyperCorros x86\n");
    } else if (strcmp(cmd, "ls") == 0) {
        term_print("/bin/   /etc/   /tmp/   /dev/\n");
        term_print("hello.cor  readme.txt  system.cfg\n");
    } else if (strcmp(cmd, "") != 0) {
        term_print("Unknown command: ");
        term_print(cmd);
        term_print("\nType 'help' for available commands.\n");
    }
}

static void term_on_draw(int id, int cx, int cy, int cw, int ch) {
    (void)id;
    int line_h = FONT_HEIGHT + 2;
    int visible_lines = ch / line_h;
    if (visible_lines < 1) visible_lines = 1;

    /* Background */
    fb_fill_rect(cx, cy, cw, ch, FB_RGB(15, 15, 22));

    /* Output lines */
    int start = term_line_count > visible_lines - 1 ? term_line_count - (visible_lines - 1) : 0;
    int y = cy + 2;
    for (int i = start; i < term_line_count && y + line_h < cy + ch; i++) {
        fb_draw_string(cx + 6, y, term_lines[i], FB_RGB(180, 200, 180), FB_RGB(15, 15, 22), 1);
        y += line_h;
    }

    /* Prompt + input */
    y = cy + ch - line_h - 4;
    fb_draw_string(cx + 6, y, "$ ", COL_ACCENT, FB_RGB(15, 15, 22), 1);
    fb_draw_string(cx + 6 + fb_char_width(1) * 2, y, term_input,
                   COL_TEXT_WHITE, FB_RGB(15, 15, 22), 1);

    /* Cursor */
    if (term_cursor_blink) {
        int cx_pos = cx + 6 + fb_char_width(1) * (2 + term_input_len);
        fb_fill_rect(cx_pos, y, fb_char_width(1), FONT_HEIGHT, COL_ACCENT);
    }
    term_cursor_blink = !term_cursor_blink;
}

static void term_on_key(int id, char key) {
    (void)id;
    if (key == '\n') {
        term_input[term_input_len] = '\0';
        term_execute(term_input);
        term_input_len = 0;
        term_input[0] = '\0';
    } else if (key == '\b') {
        if (term_input_len > 0) {
            term_input_len--;
            term_input[term_input_len] = '\0';
        }
    } else if (key == 3) { /* Ctrl+C */
        term_print("^C\n");
        term_input_len = 0;
        term_input[0] = '\0';
    } else {
        if (term_input_len < TERM_INPUT_LEN - 1) {
            term_input[term_input_len++] = key;
            term_input[term_input_len] = '\0';
        }
    }
}

void app_terminal_create(void) {
    if (term_win >= 0 && comp.windows[term_win].active) {
        gui_focus_window(term_win);
        return;
    }
    int w = 640, h = 400;
    int x = ((int)fb.width - w) / 2;
    int y = ((int)fb.height - h) / 2;
    term_win = gui_create_window(x, y, w, h, "Terminal");
    if (term_win >= 0) {
        term_line_count = 1;
        memset(term_lines[0], 0, TERM_LINE_LEN);
        strcpy(term_lines[0], "Welcome to croOS Terminal v4.0");
        term_print("Type 'help' for available commands.\n");
        gui_set_draw_callback(term_win, term_on_draw);
        gui_set_key_callback(term_win, term_on_key);
        gui_set_click_callback(term_win, 0);
    }
}
