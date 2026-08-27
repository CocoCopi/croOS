/* croOS mouse.c - PS/2 Mouse Driver
 * Reads packets from port 0x60 via IRQ12, handles mouse protocol,
 * tracks cursor position and button state, supports scroll wheel. */

#include "kernel/types.h"
#include "mouse.h"
#include "vga.h"
#include "kernel/idt.h"

static mouse_state_t mouse;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_buf[3];
static uint8_t enabled = 1;
static int cursor_x = 40;
static int cursor_y = 12;
static uint8_t cursor_visible = 1;

/* Send command to mouse controller */
static void mouse_wait(uint8_t a) {
    for (int i = 0; i < 10000; i++) {
        if ((inb(0x64) & a) == 0) return;
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(0x02);
    outb(0x64, 0xD4);
    mouse_wait(0x02);
    outb(0x60, data);
}

static uint8_t mouse_read(void) {
    mouse_wait(0x01);
    return inb(0x60);
}

static void mouse_irq(regs_t *regs) {
    (void)regs;
    uint8_t data = inb(0x60);

    mouse_buf[mouse_cycle] = data;
    mouse_cycle = (mouse_cycle + 1) % 3;

    if (mouse_cycle == 0) {
        /* Full packet received */
        mouse.buttons = mouse_buf[0] & 0x07;
        mouse.dx = (int8_t)mouse_buf[1];
        mouse.dy = -(int8_t)mouse_buf[2];  /* Invert Y */

        mouse.x += mouse.dx;
        mouse.y += mouse.dy;

        /* Clamp to screen */
        if (mouse.x < 0) mouse.x = 0;
        if (mouse.x >= VGA_WIDTH) mouse.x = VGA_WIDTH - 1;
        if (mouse.y < 0) mouse.y = 0;
        if (mouse.y >= VGA_HEIGHT) mouse.y = VGA_HEIGHT - 1;

        /* Draw cursor */
        if (cursor_visible && enabled) {
            /* Restore old position */
            vga_put_at(cursor_y, cursor_x, ' ', 0x0F);
            cursor_x = mouse.x;
            cursor_y = mouse.y;
            /* Draw new cursor */
            vga_put_at(cursor_y, cursor_x, '*', 0x0C);
        }
    }
}

void mouse_init(void) {
    mouse.x = 40;
    mouse.y = 12;
    mouse.buttons = 0;
    mouse.dx = 0;
    mouse.dy = 0;
    mouse_cycle = 0;

    /* Enable auxiliary device (mouse) */
    mouse_wait(0x02);
    outb(0x64, 0xA8);
    mouse_wait(0x02);

    /* Enable IRQ12 */
    mouse_wait(0x02);
    outb(0x64, 0x20);
    mouse_wait(0x01);
    uint8_t status = inb(0x60);
    status |= 0x02;  /* Enable IRQ12 */
    status &= ~0x20; /* Disable mouse clock */
    mouse_wait(0x02);
    outb(0x64, 0x60);
    mouse_wait(0x02);
    outb(0x60, status);

    /* Reset mouse */
    mouse_write(0xFF);
    mouse_read();

    /* Set defaults */
    mouse_write(0xF6);
    mouse_read();

    /* Enable data reporting */
    mouse_write(0xF4);
    mouse_read();

    /* Register IRQ12 (INT 44) */
    isr_install_handler(44, mouse_irq);

    /* Enable IRQ12 on PIC */
    outb(0xA1, inb(0xA1) & ~0x10);
}

mouse_state_t mouse_get_state(void) {
    return mouse;
}

void mouse_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    mouse.x = x;
    mouse.y = y;
}

void mouse_show(int show) {
    cursor_visible = show;
    if (!show) {
        vga_put_at(cursor_y, cursor_x, ' ', 0x0F);
    }
}
