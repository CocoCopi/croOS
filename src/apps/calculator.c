/* croOS calculator.c - GUI Calculator app for HyperCorros */

#include "kernel/types.h"
#include "kernel/compositor.h"
#include "drivers/framebuffer.h"
#include "kernel/font.h"
#include "string.h"

static int calc_win = -1;
static char calc_display[32] = "0";
static int calc_display_len = 1;
static double calc_a = 0;
static double calc_b = 0;
static int calc_op = 0; /* 0=none, 1=+, 2=-, 3=*, 4=/ */
static int calc_clear_next = 0;

/* Button layout: 4 columns, 5 rows */
static const char *calc_btns[5][4] = {
    {"C",  "+/-", "%",  "/"},
    {"7",  "8",   "9",  "*"},
    {"4",  "5",   "6",  "-"},
    {"1",  "2",   "3",  "+"},
    {"0",  ".",    "=",  "="}
};
static int calc_btn_colors[5][4] = {
    {3, 1, 1, 2}, {0, 0, 0, 2}, {0, 0, 0, 2},
    {0, 0, 0, 2}, {0, 0, 0, 2}
};
/* 0=dark, 1=dim, 2=accent, 3=red */

static uint32_t btn_color(int type) {
    switch (type) {
        case 0: return FB_RGB(55, 55, 70);
        case 1: return FB_RGB(40, 40, 55);
        case 2: return COL_ACCENT;
        case 3: return COL_BTN_CLOSE;
        default: return FB_RGB(55, 55, 70);
    }
}

static void calc_update_display(void) {
    /* Truncate display */
    if (calc_display_len > 20) calc_display_len = 20;
    calc_display[calc_display_len] = '\0';
}

static void calc_number(const char *num) {
    if (calc_clear_next) {
        calc_display_len = 0;
        calc_display[0] = '\0';
        calc_clear_next = 0;
    }
    if (strcmp(num, ".") == 0) {
        /* Check if already has decimal */
        for (int i = 0; i < calc_display_len; i++)
            if (calc_display[i] == '.') return;
    }
    if (calc_display_len == 1 && calc_display[0] == '0' && strcmp(num, ".") != 0) {
        calc_display_len = 0;
    }
    int sl = strlen(num);
    for (int i = 0; i < sl && calc_display_len < 20; i++)
        calc_display[calc_display_len++] = num[i];
    calc_display[calc_display_len] = '\0';
}

static double calc_atof(const char *s) {
    double result = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') result = result * 10 + (*s++ - '0');
    if (*s == '.') { s++; double f = 1; while (*s >= '0' && *s <= '9') { f /= 10; result += (*s++ - '0') * f; } }
    return result * sign;
}

static void calc_toa(double val, char *buf) {
    if (val < 0) { *buf++ = '-'; val = -val; }
    int whole = (int)val;
    double frac = val - whole;
    char tmp[16]; int ti = 0;
    if (whole == 0) { tmp[ti++] = '0'; }
    else { while (whole) { tmp[ti++] = '0' + whole % 10; whole /= 10; } }
    for (int i = ti - 1; i >= 0; i--) *buf++ = tmp[i];
    if (frac > 0.0001) {
        *buf++ = '.';
        for (int i = 0; i < 6; i++) {
            frac *= 10;
            int d = (int)frac;
            *buf++ = '0' + d;
            frac -= d;
            if (frac < 0.0001) break;
        }
    }
    *buf = '\0';
}

static void calc_do_op(void) {
    calc_b = calc_atof(calc_display);
    double result = 0;
    switch (calc_op) {
        case 1: result = calc_a + calc_b; break;
        case 2: result = calc_a - calc_b; break;
        case 3: result = calc_a * calc_b; break;
        case 4: result = calc_b != 0 ? calc_a / calc_b : 0; break;
    }
    calc_toa(result, calc_display);
    calc_display_len = strlen(calc_display);
    calc_op = 0;
}

static void calc_button(int row, int col) {
    const char *label = calc_btns[row][col];
    if (label[0] >= '0' && label[0] <= '9') {
        calc_number(label);
    } else if (strcmp(label, ".") == 0) {
        calc_number(".");
    } else if (strcmp(label, "C") == 0) {
        calc_display[0] = '0'; calc_display_len = 1;
        calc_display[1] = '\0'; calc_a = 0; calc_b = 0; calc_op = 0;
    } else if (strcmp(label, "+/-") == 0) {
        if (calc_display[0] == '-') {
            for (int i = 1; i <= calc_display_len; i++) calc_display[i-1] = calc_display[i];
            calc_display_len--;
        } else {
            for (int i = calc_display_len; i >= 0; i--) calc_display[i+1] = calc_display[i];
            calc_display[0] = '-'; calc_display_len++;
        }
    } else if (strcmp(label, "%") == 0) {
        calc_a = calc_atof(calc_display) / 100.0;
        calc_toa(calc_a, calc_display);
        calc_display_len = strlen(calc_display);
    } else if (strcmp(label, "+") == 0) { calc_a = calc_atof(calc_display); calc_op = 1; calc_clear_next = 1; }
      else if (strcmp(label, "-") == 0) { calc_a = calc_atof(calc_display); calc_op = 2; calc_clear_next = 1; }
      else if (strcmp(label, "*") == 0) { calc_a = calc_atof(calc_display); calc_op = 3; calc_clear_next = 1; }
      else if (strcmp(label, "/") == 0) { calc_a = calc_atof(calc_display); calc_op = 4; calc_clear_next = 1; }
      else if (strcmp(label, "=") == 0) { calc_do_op(); calc_clear_next = 1; }
}

static void calc_on_draw(int id, int cx, int cy, int cw, int ch) {
    (void)id;
    fb_fill_rect(cx, cy, cw, ch, COL_WIN_BG);

    int pad = 8;
    int display_h = 48;
    int display_y = cy + pad;

    /* Display background */
    fb_fill_rounded_rect(cx + pad, display_y, cw - pad * 2, display_h, 8, FB_RGB(18, 18, 28));

    /* Display text (right-aligned) */
    int text_w = fb_text_width(calc_display, 2);
    fb_draw_string(cx + cw - pad - text_w, display_y + (display_h - fb_char_height(2)) / 2,
                   calc_display, COL_TEXT_WHITE, FB_RGB(18, 18, 28), 2);

    /* Operator indicator */
    if (calc_op > 0) {
        const char *ops[] = {"", "+", "-", "*", "/"};
        fb_draw_string(cx + pad + 4, display_y + 4, ops[calc_op], COL_ACCENT, FB_RGB(18, 18, 28), 1);
    }

    /* Buttons */
    int btn_area_y = display_y + display_h + pad;
    int btn_area_h = ch - display_h - pad * 3;
    int btn_w = (cw - pad * 5) / 4;
    int btn_h = (btn_area_h - pad * 4) / 5;

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = cx + pad + c * (btn_w + pad);
            int by = btn_area_y + r * (btn_h + pad);
            uint32_t bg = btn_color(calc_btn_colors[r][c]);
            fb_fill_rounded_rect(bx, by, btn_w, btn_h, 8, bg);

            /* Button label */
            const char *label = calc_btns[r][c];
            int lw;
            if (strcmp(label, "+/-") == 0) lw = fb_text_width("+/-", 1);
            else lw = fb_text_width(label, (r == 0 && c == 0) ? 1 : 2);
            int scale = (r == 0 && c == 0) ? 1 : 2;
            if (strcmp(label, "=") == 0) scale = 2;
            lw = fb_text_width(label, scale);
            fb_draw_string(bx + (btn_w - lw) / 2, by + (btn_h - fb_char_height(scale)) / 2,
                           label, COL_TEXT_WHITE, bg, scale);
        }
    }
}

static void calc_on_click(int id, int mx, int my) {
    (void)id;
    int pad = 8;
    int display_h = 48;
    int btn_area_y = pad + display_h + pad;
    int cw = comp.windows[id].w;
    int ch = comp.windows[id].h;
    int btn_area_h = ch - display_h - pad * 3;
    int btn_w = (cw - pad * 5) / 4;
    int btn_h = (btn_area_h - pad * 4) / 5;

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = pad + c * (btn_w + pad);
            int by = btn_area_y + r * (btn_h + pad);
            if (mx >= bx && mx < bx + btn_w && my >= by && my < by + btn_h) {
                calc_button(r, c);
                return;
            }
        }
    }
}

void app_calculator_create(void) {
    if (calc_win >= 0 && comp.windows[calc_win].active) {
        gui_focus_window(calc_win);
        return;
    }
    int w = 320, h = 440;
    int x = ((int)fb.width - w) / 2 + 40;
    int y = ((int)fb.height - h) / 2;
    calc_win = gui_create_window(x, y, w, h, "Calculator");
    if (calc_win >= 0) {
        calc_display[0] = '0'; calc_display[1] = '\0'; calc_display_len = 1;
        gui_set_draw_callback(calc_win, calc_on_draw);
        gui_set_click_callback(calc_win, calc_on_click);
        gui_set_key_callback(calc_win, 0);
    }
}
