// croOS App Framework — include/app.h
// C header for apps compiled from Corros via --compile
#ifndef CORROS_APP_H
#define CORROS_APP_H
#include <stdint.h>
// VGA framebuffer at 0xB8000
#define CORROS_VGA 0xB8000
#define CORROS_COLS 80
#define CORROS_ROWS 25
// Keyboard I/O ports
#define CORROS_KBD_PORT 0x60
#define CORROS_KBD_STATUS 0x64
// App color attributes
#define CORROS_COLOR_WHITE 0x07
#define CORROS_COLOR_BRIGHT 0x0F
#define CORROS_COLOR_YELLOW 0x0E
#define CORROS_COLOR_RED 0x0C
#define CORROS_COLOR_GREEN 0x0A
#define CORROS_COLOR_BLUE 0x09
#define CORROS_COLOR_CYAN 0x0B
#define CORROS_COLOR_TITLE 0x1F
#define CORROS_COLOR_STATUS 0x70
#endif
