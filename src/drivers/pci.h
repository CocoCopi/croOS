/* croOS pci.h - PCI bus enumeration and device discovery */
#ifndef _PCI_H
#define _PCI_H

#include "kernel/types.h"

#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

#define PCI_VENDOR_ID    0x00
#define PCI_DEVICE_ID    0x02
#define PCI_COMMAND      0x04
#define PCI_STATUS       0x06
#define PCI_REVISION     0x08
#define PCI_PROG_IF      0x09
#define PCI_SUBCLASS     0x0A
#define PCI_CLASS        0x0B
#define PCI_CACHE_LINE   0x0C
#define PCI_LATENCY      0x0D
#define PCI_HEADER_TYPE  0x0E
#define PCI_BIST         0x0F
#define PCI_BAR0         0x10
#define PCI_BAR1         0x14
#define PCI_BAR2         0x18
#define PCI_BAR3         0x1C
#define PCI_BAR4         0x20
#define PCI_BAR5         0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN  0x3D

#define PCI_CLASS_UNCLASSIFIED     0x00
#define PCI_CLASS_MASS_STORAGE     0x01
#define PCI_CLASS_NETWORK          0x02
#define PCI_CLASS_BRIDGE           0x06

#define PCI_MASS_STORAGE_AHCI      0x06
#define PCI_NETWORK_ETHERNET        0x00

typedef struct {
    uint8_t  bus;
    uint8_t  slot;
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  revision;
    uint8_t  irq;
    uint32_t bar[6];
    uint32_t bar_size[6];
    char     name[64];
} pci_device_t;

#define PCI_MAX_DEVICES 64

void pci_init(void);
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint8_t  pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
pci_device_t *pci_get_device(uint8_t class, uint8_t subclass);
pci_device_t *pci_get_all(int *count);
void pci_enable_bus_master(pci_device_t *dev);
void pci_dump_devices(void);

#endif
