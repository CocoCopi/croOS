/* croOS serial.c — COM1 serial port driver
 * Used for QEMU debug output (-serial stdio) and kernel logging. */

#include "kernel/types.h"
#include "drivers/serial.h"

static inline int serial_tx_ready(void) {
    return inb(SERIAL_COM1 + 5) & 0x20;
}

void serial_init(void) {
    outb(SERIAL_COM1 + 1, 0x00);  /* Disable interrupts */
    outb(SERIAL_COM1 + 3, 0x80);  /* Enable DLAB (set baud rate divisor) */
    outb(SERIAL_COM1 + 0, 0x03);  /* Divisor lo: 38400 baud */
    outb(SERIAL_COM1 + 1, 0x00);  /* Divisor hi */
    outb(SERIAL_COM1 + 3, 0x03);  /* 8 bits, no parity, 1 stop bit */
    outb(SERIAL_COM1 + 2, 0xC7);  /* Enable FIFO */
    outb(SERIAL_COM1 + 4, 0x0B);  /* IRQs enabled, RTS/DSR set */
}

void serial_putchar(char c) {
    while (!serial_tx_ready());
    outb(SERIAL_COM1, (uint8_t)c);
}

void serial_puts(const char *str) {
    while (*str) {
        if (*str == '\n') serial_putchar('\r');
        serial_putchar(*str++);
    }
}

void serial_put_hex(uint32_t n) {
    serial_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nib = (n >> i) & 0xF;
        serial_putchar(nib < 10 ? '0' + nib : 'A' + nib - 10);
    }
}

void serial_put_dec(uint32_t n) {
    if (n == 0) { serial_putchar('0'); return; }
    char buf[12]; int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) serial_putchar(buf[--i]);
}
