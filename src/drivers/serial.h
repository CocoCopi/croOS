/* croOS serial.h — COM1 serial port driver (for QEMU debug output) */
#ifndef _SERIAL_H
#define _SERIAL_H

#include "kernel/types.h"

#define SERIAL_COM1 0x3F8

void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char *str);
void serial_put_hex(uint32_t n);
void serial_put_dec(uint32_t n);

#endif
