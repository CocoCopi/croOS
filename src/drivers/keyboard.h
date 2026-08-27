/* croOS keyboard.h — PS/2 keyboard driver */
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "kernel/types.h"

#define KB_BUFFER_SIZE 256

/* Exposed for compositor non-blocking polling */
extern volatile char kb_buffer[KB_BUFFER_SIZE];
extern volatile int  kb_head;
extern volatile int  kb_tail;

void     kb_init(void);
char     kb_getchar(void);
uint8_t  kb_scancode(void);
int      kb_has_input(void);
uint8_t  kb_get_modifiers(void);  /* shift=1, ctrl=2, alt=4 */
void     kb_clear_buffer(void);

#endif
