/* croOS mouse.h - PS/2 mouse driver */
#ifndef _MOUSE_H
#define _MOUSE_H

#include "kernel/types.h"

typedef struct {
    int32_t  x;
    int32_t  y;
    uint8_t  buttons;  /* bit 0=left, 1=right, 2=middle */
    int8_t   dx;
    int8_t   dy;
    int8_t   dz;       /* scroll wheel */
} mouse_state_t;

void mouse_init(void);
mouse_state_t mouse_get_state(void);
void mouse_set_cursor(int x, int y);
void mouse_show(int show);

#endif
