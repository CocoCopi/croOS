/* croOS pci.c - PCI bus enumeration
 * Scans all 256 buses, 32 slots, 8 functions for PCI devices.
 * Reads config space to identify vendors, classes, BARs, IRQs. */

#include "kernel/types.h"
#include "pci.h"
#include "vga.h"
#include "string.h"

static pci_device_t pci_devices[PCI_MAX_DEVICES];
static int pci_count = 0;

static const char *pci_class_names[] = {
    "Legacy Device", "VGA Display", "Other Display",
    "Mass Storage Controller", "Network Controller",
    "Bridge Device", "Communication Controller",
    "System Peripheral", "Input Device", "Docking Station",
    "Processor", "Serial Bus Controller", "Wireless",
    "Intelligent I/O", "Satellite", "Crypto", "Signal Processing"
};

static const char *pci_subclass_storage[] = {
    "SCSI", "IDE", "Floppy", "IPI", "RAID",
    "ATA (ADMA)", "SATA (AHCI)", "SAS", "NVM Express", "UFS"
};

static const char *pci_subclass_net[] = {
    "Ethernet", "Token Ring", "FDDI", "ATM",
    "ISDN", "WorldFip", "PICMG", "Infiniband",
    "Fabric", "Other Network"
};

static uint32_t pci_make_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (1 << 31) | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
           ((uint32_t)func << 8) | (offset & 0xFC);
}

uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_make_addr(bus, slot, func, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = pci_make_addr(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDR, addr);
    uint16_t val = (uint16_t)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return val;
}

uint8_t pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = pci_make_addr(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDR, addr);
    return (uint8_t)((inl(PCI_CONFIG_DATA) >> ((offset & 3) * 8)) & 0xFF);
}

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    outl(PCI_CONFIG_ADDR, pci_make_addr(bus, slot, func, offset));
    outl(PCI_CONFIG_DATA, val);
}

void pci_enable_bus_master(pci_device_t *dev) {
    uint16_t cmd = pci_read_word(dev->bus, dev->slot, dev->func, PCI_COMMAND);
    cmd |= 0x04;  /* Bus Master Enable */
    pci_write_dword(dev->bus, dev->slot, dev->func, PCI_COMMAND, cmd);
}

static uint32_t pci_get_bar_size(pci_device_t *dev, int bar_num) {
    uint8_t offset = PCI_BAR0 + bar_num * 4;
    uint32_t bar_orig = pci_read_dword(dev->bus, dev->slot, dev->func, offset);

    /* Write all 1s to get the size */
    pci_write_dword(dev->bus, dev->slot, dev->func, offset, 0xFFFFFFFF);
    uint32_t size = pci_read_dword(dev->bus, dev->slot, dev->func, offset);

    /* Restore original value */
    pci_write_dword(dev->bus, dev->slot, dev->func, offset, bar_orig);

    if (size == 0) return 0;
    if (bar_orig & 1) {
        /* I/O port BAR */
        return (size & ~3) + 1;
    } else {
        /* Memory BAR */
        return ~(size & ~0xF) + 1;
    }
}

static void pci_get_device_name(pci_device_t *dev) {
    /* Vendor name lookup */
    switch (dev->vendor_id) {
        case 0x8086: strcpy(dev->name, "Intel "); break;
        case 0x10EC: strcpy(dev->name, "Realtek "); break;
        case 0x10DE: strcpy(dev->name, "NVIDIA "); break;
        case 0x1002: strcpy(dev->name, "AMD/ATI "); break;
        case 0x1022: strcpy(dev->name, "AMD "); break;
        case 0x1039: strcpy(dev->name, "SiS "); break;
        case 0x1106: strcpy(dev->name, "VIA "); break;
        case 0x1033: strcpy(dev->name, "NEC "); break;
        case 0x1133: strcpy(dev->name, "Lucent "); break;
        case 0x14E4: strcpy(dev->name, "Broadcom "); break;
        case 0x10B7: strcpy(dev->name, "3Com "); break;
        case 0x1050: strcpy(dev->name, "Winbond "); break;
        default: strcpy(dev->name, "Unknown "); break;
    }

    char suffix[32];
    switch (dev->class) {
        case PCI_CLASS_MASS_STORAGE:
            if (dev->subclass < 10)
                strcpy(suffix, pci_subclass_storage[dev->subclass]);
            else strcpy(suffix, "Storage");
            break;
        case PCI_CLASS_NETWORK:
            if (dev->subclass < 10)
                strcpy(suffix, pci_subclass_net[dev->subclass]);
            else strcpy(suffix, "Network");
            break;
        case PCI_CLASS_BRIDGE:
            strcpy(suffix, "PCI Bridge");
            break;
        default:
            if (dev->class < 17) strcpy(suffix, pci_class_names[dev->class]);
            else strcpy(suffix, "Unknown Class");
            break;
    }
    strcat(dev->name, suffix);
}

static void pci_scan_bus(uint8_t bus);

static void pci_scan_function(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor = pci_read_word(bus, slot, func, PCI_VENDOR_ID);
    if (vendor == 0xFFFF) return;

    pci_device_t *dev = &pci_devices[pci_count];
    memset(dev, 0, sizeof(pci_device_t));

    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;
    dev->vendor_id = vendor;
    dev->device_id = pci_read_word(bus, slot, func, PCI_DEVICE_ID);
    dev->class = pci_read_byte(bus, slot, func, PCI_CLASS);
    dev->subclass = pci_read_byte(bus, slot, func, PCI_SUBCLASS);
    dev->prog_if = pci_read_byte(bus, slot, func, PCI_PROG_IF);
    dev->revision = pci_read_byte(bus, slot, func, PCI_REVISION);
    dev->irq = pci_read_byte(bus, slot, func, PCI_INTERRUPT_LINE);

    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_read_dword(bus, slot, func, PCI_BAR0 + i * 4);
        dev->bar_size[i] = pci_get_bar_size(dev, i);
    }

    pci_get_device_name(dev);

    /* Check for PCI-to-PCI bridge */
    if (dev->class == PCI_CLASS_BRIDGE && dev->subclass == 0x04) {
        uint8_t secondary = pci_read_byte(bus, slot, func, 0x19);
        if (secondary) pci_scan_bus(secondary);
    }

    pci_count++;
}

static void pci_scan_slot(uint8_t bus, uint8_t slot) {
    uint16_t vendor = pci_read_word(bus, slot, 0, PCI_VENDOR_ID);
    if (vendor == 0xFFFF) return;

    pci_scan_function(bus, slot, 0);
    if (pci_read_byte(bus, slot, 0, PCI_HEADER_TYPE) & 0x80) {
        for (int func = 1; func < 8; func++) {
            if (pci_read_word(bus, slot, func, PCI_VENDOR_ID) != 0xFFFF)
                pci_scan_function(bus, slot, func);
        }
    }
}

static void pci_scan_bus(uint8_t bus) {
    for (int slot = 0; slot < 32; slot++)
        pci_scan_slot(bus, slot);
}

void pci_init(void) {
    pci_count = 0;
    memset(pci_devices, 0, sizeof(pci_devices));

    /* Check if bus 0 has multiple functions (PCI-to-PCI bridges) */
    uint8_t header_type = pci_read_byte(0, 0, 0, PCI_HEADER_TYPE);
    if (header_type & 0x80) {
        for (int func = 0; func < 8; func++) {
            if (pci_read_word(0, 0, func, PCI_VENDOR_ID) != 0xFFFF)
                pci_scan_function(0, 0, func);
        }
    } else {
        pci_scan_bus(0);
    }
}

pci_device_t *pci_get_device(uint8_t class, uint8_t subclass) {
    for (int i = 0; i < pci_count; i++) {
        if (pci_devices[i].class == class && pci_devices[i].subclass == subclass)
            return &pci_devices[i];
    }
    return NULL;
}

pci_device_t *pci_get_all(int *count) {
    *count = pci_count;
    return pci_devices;
}

void pci_dump_devices(void) {
    vga_set_color(VGA_LBLUE, VGA_BLACK);
    vga_puts("  PCI Device Enumeration:\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  ──────────────────────────────────────────────────\n");
    vga_puts("  Bus:Slot:Fn  Vendor:Device  Class    IRQ  Name\n");
    vga_puts("  ──────────────────────────────────────────────────\n");
    for (int i = 0; i < pci_count; i++) {
        pci_device_t *d = &pci_devices[i];
        vga_puts("  ");
        vga_put_dec(d->bus);
        vga_putchar(':');
        vga_put_dec(d->slot);
        vga_putchar(':');
        vga_put_dec(d->func);
        vga_puts("   ");
        vga_put_hex(((uint32_t)d->vendor_id << 16) | d->device_id);
        vga_puts("   ");
        vga_put_hex(((uint32_t)d->class << 8) | d->subclass);
        vga_puts("     ");
        vga_put_dec(d->irq);
        vga_puts("  ");
        vga_puts(d->name);
        vga_putchar('\n');
    }
    vga_puts("  Total: ");
    vga_put_dec(pci_count);
    vga_puts(" devices\n");
}
