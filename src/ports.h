#ifndef CORROS_PORTS_H
#define CORROS_PORTS_H
#include <stdint.h>
uint8_t inb(uint16_t port);
void    outb(uint16_t port, uint8_t val);
#endif
