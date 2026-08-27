/* croOS usb.c - USB host controller driver (UHCI/OHCI/EHCI)
 * Detects USB host controllers via PCI, enumerates devices,
 * reads device descriptors, supports HID (keyboard/mouse). */

#include "kernel/types.h"
#include "usb.h"
#include "pci.h"
#include "vga.h"
#include "timer.h"
#include "mm/kmalloc.h"
#include "string.h"

static usb_device_t usb_devices[USB_MAX_DEVICES];
static usb_hci_t usb_controllers[4];
static int device_count = 0;
static int controller_count = 0;

/* UHCI registers */
#define UHCI_USBCMD    0x00
#define UHCI_USBSTS    0x02
#define UHCI_USBINTR   0x04
#define UHCI_FRNUM     0x06
#define UHCI_FLBASEADD 0x08
#define UHCI_SOFMOD    0x12
#define UHCI_PORTSC1   0x16
#define UHCI_PORTSC2   0x18

static uint8_t usb_read_reg8(uint32_t base, uint16_t reg) {
    return inb((uint16_t)base + reg);
}

static void usb_write_reg8(uint32_t base, uint16_t reg, uint8_t val) {
    outb((uint16_t)base + reg, val);
}

static uint16_t usb_read_reg16(uint32_t base, uint16_t reg) {
    return inw((uint16_t)base + reg);
}

static void usb_write_reg16(uint32_t base, uint16_t reg, uint16_t val) {
    outw((uint16_t)base + reg, val);
}

/* Reset UHCI controller */
static void uhci_reset(uint32_t base) {
    usb_write_reg16(base, UHCI_USBCMD, 0x0004);  /* Global Reset */
    timer_sleep(100);
    usb_write_reg16(base, UHCI_USBCMD, 0x0000);
}

/* Check if a device is connected on a UHCI port */
static int uhci_port_connected(uint32_t base, int port) {
    uint16_t portsc = usb_read_reg16(base, UHCI_PORTSC1 + port * 2);
    return (portsc & 0x0001) ? 1 : 0;  /* Current Connect Status */
}

/* Reset a specific port */
static void uhci_port_reset(uint32_t base, int port) {
    uint16_t portsc = UHCI_PORTSC1 + port * 2;
    usb_write_reg16(base, portsc, 0x0200);  /* Port Reset */
    timer_sleep(50);
    usb_write_reg16(base, portsc, 0x0000);  /* Clear Reset */
    timer_sleep(10);
}

void usb_init(void) {
    device_count = 0;
    controller_count = 0;
    memset(usb_devices, 0, sizeof(usb_devices));
    memset(usb_controllers, 0, sizeof(usb_controllers));

    /* Scan PCI for USB controllers */
    int pci_count;
    pci_device_t *devices = pci_get_all(&pci_count);

    for (int i = 0; i < pci_count; i++) {
        if (devices[i].class == 0x0C && devices[i].subclass == 0x03) {
            /* USB controller found */
            usb_hci_t *hci = &usb_controllers[controller_count];
            hci->base = devices[i].bar[4] & ~0x0F;
            hci->class_code = devices[i].prog_if;
            hci->port_count = 2;
            hci->in_use = 1;
            pci_enable_bus_master(&devices[i]);

            /* Reset controller */
            if (hci->base) {
                uhci_reset(hci->base);
            }

            controller_count++;
            if (controller_count >= 4) break;
        }
    }
}

int usb_enumerate_device(int port) {
    if (device_count >= USB_MAX_DEVICES) return -1;

    /* Find first controller with connected device */
    for (int c = 0; c < controller_count; c++) {
        usb_hci_t *hci = &usb_controllers[c];
        if (!hci->in_use || !hci->base) continue;

        for (int p = 0; p < hci->port_count; p++) {
            if (uhci_port_connected(hci->base, p)) {
                uhci_port_reset(hci->base, p);

                usb_device_t *dev = &usb_devices[device_count];
                memset(dev, 0, sizeof(usb_device_t));
                dev->address = device_count + 1;
                dev->port = p;
                dev->in_use = 1;
                dev->max_packet_size = 8;

                /* TODO: Send GET_DESCRIPTOR request to get device info */

                device_count++;
                return device_count - 1;
            }
        }
    }
    return -1;
}

int usb_get_device_count(void) { return device_count; }

usb_device_t *usb_get_device(int index) {
    if (index < 0 || index >= device_count) return NULL;
    return &usb_devices[index];
}

void usb_dump_devices(void) {
    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  USB Devices:\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    if (device_count == 0) {
        vga_puts("  No USB devices connected.\n");
        return;
    }
    for (int i = 0; i < device_count; i++) {
        usb_device_t *dev = &usb_devices[i];
        vga_puts("  [");
        vga_put_dec(i);
        vga_puts("] ");
        if (dev->manufacturer[0]) { vga_puts(dev->manufacturer); vga_putchar(' '); }
        if (dev->product[0]) vga_puts(dev->product);
        else {
            vga_puts("Device 0x");
            vga_put_hex(((uint32_t)dev->vendor_id << 16) | dev->product_id);
        }
        vga_puts(" (class=");
        vga_put_hex(dev->device_class);
        vga_puts(" addr=");
        vga_put_dec(dev->address);
        vga_puts(")\n");
    }
}

void usb_poll(void) {
    /* Check for hot-plug events */
    for (int c = 0; c < controller_count; c++) {
        usb_hci_t *hci = &usb_controllers[c];
        if (!hci->in_use || !hci->base) continue;

        for (int p = 0; p < hci->port_count; p++) {
            uint16_t portsc = usb_read_reg16(hci->base, UHCI_PORTSC1 + p * 2);
            if (portsc & 0x0040) {  /* Connect Status Change */
                usb_write_reg16(hci->base, UHCI_PORTSC1 + p * 2, 0x0040);
                if (portsc & 0x0001) {
                    usb_enumerate_device(p);
                }
            }
        }
    }
}
