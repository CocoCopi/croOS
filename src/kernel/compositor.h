/* croOS compositor.h - HyperCorros GUI Window Manager
 * Full GUI compositor: desktop, windows, mouse cursor, taskbar.
 * Uses linear framebuffer for pixel-level rendering. */
#ifndef _COMPOSITOR_H
#define _COMPOSITOR_H

#include "kernel/types.h"
#include "drivers/framebuffer.h"

/* Globals exposed for apps */
extern framebuffer_t fb_global;
#define fb fb_global

#define GUI_MAX_WINDOWS  32
#define GUI_MAX_TITLE    64
#define GUI_TASKBAR_H    36
#define GUI_TITLEBAR_H   28
#define GUI_SHADOW_SIZE  8
#define GUI_BORDER_R     12

/* Title bar button IDs */
#define BTN_CLOSE   0
#define BTN_MINIMIZE 1
#define BTN_MAXIMIZE 2

/* Color palette */
#define COL_DESKTOP_BG    FB_RGB(12, 12, 20)
#define COL_TASKBAR_BG    FB_RGB(25, 25, 35)
#define COL_TASKBAR_TEXT  FB_RGB(200, 200, 210)
#define COL_WIN_BG        FB_RGB(30, 30, 42)
#define COL_WIN_ACTIVE    FB_RGB(40, 40, 55)
#define COL_WIN_BORDER    FB_RGB(55, 55, 75)
#define COL_TITLE_ACTIVE  FB_RGB(35, 35, 50)
#define COL_TITLE_INACT   FB_RGB(28, 28, 38)
#define COL_BTN_CLOSE     FB_RGB(220, 60, 55)
#define COL_BTN_MIN       FB_RGB(220, 180, 40)
#define COL_BTN_MAX       FB_RGB(60, 190, 80)
#define COL_TEXT_WHITE     FB_RGB(240, 240, 245)
#define COL_TEXT_DIM       FB_RGB(140, 140, 155)
#define COL_ACCENT         FB_RGB(100, 140, 255)
#define COL_HOVER          FB_RGB(255, 255, 255)

typedef struct {
    int x, y, w, h;
    char title[GUI_MAX_TITLE];
    uint8_t active;
    uint8_t visible;
    uint8_t minimized;
    uint8_t focused;
    int drag_offset_x, drag_offset_y;
    uint8_t dragging;
    /* Content callback */
    void (*on_draw)(int win_id, int cx, int cy, int cw, int ch);
    void (*on_key)(int win_id, char key);
    void (*on_click)(int win_id, int mx, int my);
    int win_data[8]; /* app-specific data */
} gui_window_t;

typedef struct {
    gui_window_t windows[GUI_MAX_WINDOWS];
    int active_window;
    int window_count;
    int mouse_x, mouse_y;
    uint8_t mouse_left, mouse_right;
    int drag_win;
    uint8_t running;
    /* Taskbar */
    char taskbar_time[16];
} compositor_t;

/* Global compositor state */
extern compositor_t comp;

/* Init & main loop */
void compositor_init(void);
void compositor_run(void);

/* Window management */
int  gui_create_window(int x, int y, int w, int h, const char *title);
void gui_destroy_window(int id);
void gui_set_window_title(int id, const char *title);
void gui_move_window(int id, int x, int y);
void gui_resize_window(int id, int w, int h);
void gui_focus_window(int id);
void gui_minimize_window(int id);
void gui_set_draw_callback(int id, void (*cb)(int,int,int,int,int));
void gui_set_key_callback(int id, void (*cb)(int,char));
void gui_set_click_callback(int id, void (*cb)(int,int,int));
int  gui_get_window_data(int id, int idx);
void gui_set_window_data(int id, int idx, int val);

/* Drawing */
void gui_draw_desktop(void);
void gui_draw_taskbar(void);
void gui_draw_window(int id);
void gui_draw_mouse(void);
void gui_redraw_all(void);

/* Built-in apps */
void app_terminal_create(void);
void app_calculator_create(void);
void app_file_manager_create(void);
void app_text_editor_create(void);
void app_about_create(void);

#endif
