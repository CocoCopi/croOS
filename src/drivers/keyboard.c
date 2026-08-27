/* croOS keyboard.c — PS/2 Keyboard Driver
 * Handles scancode set 1, shift/ctrl/alt modifiers, buffer with ring semantics.
 * Connected to IRQ1 (INT 33). */

#include "kernel/types.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "kernel/idt.h"

static volatile char kb_buffer[KB_BUFFER_SIZE];
static volatile int  kb_head = 0;
static volatile int  kb_tail = 0;
static volatile uint8_t modifiers = 0;

/* Scancode set 1 → ASCII (unshifted / shifted) */
static const char sc1_normal[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']', '\n',
    0,  'a','s','d','f','g','h','j','k','l',';', '\'', '`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',  0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0, 0, 0, 0, 0
};

static const char sc1_shifted[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}', '\n',
    0,  'A','S','D','F','G','H','J','K','L',':', '"', '~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?',  0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0, 0, 0, 0, 0
};

static void kb_push(char c) {
    int next = (kb_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

static void kb_irq(regs_t *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);

    if (sc & 0x80) {
        /* Key release */
        uint8_t key = sc & 0x7F;
        if (key == 42 || key == 54) modifiers &= ~1;  /* shift off */
        else if (key == 29 || key == 157) modifiers &= ~2;  /* ctrl off */
        else if (key == 56 || key == 184) modifiers &= ~4;  /* alt off */
    } else {
        /* Key press */
        uint8_t key = sc;
        if (key == 42 || key == 54) modifiers |= 1;   /* shift on */
        else if (key == 29 || key == 157) modifiers |= 2;  /* ctrl on */
        else if (key == 56 || key == 184) modifiers |= 4;  /* alt on */
        else if (key == 58) { /* caps lock toggle — not fully implemented */ }
        else {
            char c;
            if (modifiers & 1) c = sc1_shifted[key];
            else c = sc1_normal[key];
            if (c) {
                /* Ctrl+C, Ctrl+D, Ctrl+Z */
                if ((modifiers & 2) && (c == 'c' || c == 'C')) kb_push(3);
                else if ((modifiers & 2) && (c == 'd' || c == 'D')) kb_push(4);
                else kb_push(c);
            }
        }
    }
}

void kb_init(void) {
    modifiers = 0;
    kb_head = 0;
    kb_tail = 0;
    isr_install_handler(33, kb_irq);
    /* Enable IRQ1 on PIC */
    outb(0x21, inb(0x21) & ~0x02);
}

char kb_getchar(void) {
    while (kb_head == kb_tail) { asm volatile("hlt"); }
    char c = (char)kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

uint8_t kb_scancode(void) {
    return inb(0x60);
}

int kb_has_input(void) {
    return kb_head != kb_tail;
}

uint8_t kb_get_modifiers(void) {
    return modifiers;
}

void kb_clear_buffer(void) {
    kb_head = 0;
    kb_tail = 0;
}
