/* croOS font.h - 8x16 bitmap VGA font
 * Standard IBM PC VGA font, 256 glyphs, 8 pixels wide, 16 pixels tall.
 * Each glyph is 16 bytes: one byte per row, MSB = leftmost pixel. */
#ifndef _FONT_H
#define _FONT_H

#include "kernel/types.h"

#define FONT_WIDTH   8
#define FONT_HEIGHT  16
#define FONT_NUM_GLYPHS 128

const uint8_t *font_get_glyph(int ch);

#endif
