/* croOS ata.c - ATA/IDE PIO disk driver
 * Supports primary/secondary controllers, master/slave devices,
 * 28-bit and 48-bit LBA addressing, read/write sectors. */

#include "kernel/types.h"
#include "ata.h"
#include "vga.h"
#include "string.h"

static ata_device_t devices[4];  /* primary master/slave, secondary master/slave */
static int device_count = 0;

static inline void ata_delay(uint16_t ctrl) {
    inb(ctrl); inb(ctrl); inb(ctrl); inb(ctrl);
}

static int ata_wait_ready(uint16_t io) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(io + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) return 0;
    }
    return -1;
}

static int ata_wait_drq(uint16_t io) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(io + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DRQ) return 0;
    }
    return -1;
}

static void ata_select(uint16_t io, uint8_t slave) {
    outb(io + ATA_REG_DRIVE, 0xA0 | (slave << 4));
    ata_delay(io);
}

static uint8_t ata_identify(uint16_t io, uint16_t ctrl, uint8_t slave, ata_device_t *dev) {
    ata_select(io, slave);
    ata_delay(ctrl);

    outb(io + ATA_REG_SECCOUNT, 0);
    outb(io + ATA_REG_LBA_LO, 0);
    outb(io + ATA_REG_LBA_MID, 0);
    outb(io + ATA_REG_LBA_HI, 0);
    outb(io + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    ata_delay(ctrl);
    uint8_t status = inb(io + ATA_REG_STATUS);
    if (status == 0) return 0;  /* No device */

    if (ata_wait_ready(io) != 0) return 0;
    if (status & ATA_SR_ERR) return 0;  /* ATAPI device */

    if (ata_wait_drq(io) != 0) return 0;

    /* Read 256 words of identification data */
    uint16_t ident[256];
    for (int i = 0; i < 256; i++)
        ident[i] = inw(io + ATA_REG_DATA);

    dev->present = 1;
    dev->io_base = io;
    dev->ctrl_base = ctrl;
    dev->slave = slave;

    /* Extract model name (40 chars at offset 27-46 = bytes 54-91) */
    for (int i = 0; i < 20; i++) {
        dev->model[i*2]     = (char)(ident[ATA_IDENT_MODEL/2 + i] >> 8);
        dev->model[i*2 + 1] = (char)(ident[ATA_IDENT_MODEL/2 + i] & 0xFF);
    }
    dev->model[40] = '\0';

    /* Extract CHS geometry */
    dev->cylinders = ident[1];
    dev->heads = ident[3];
    dev->sectors = ident[6];

    /* Check for LBA48 support */
    uint32_t cmd_sets = ((uint32_t)ident[ATA_IDENT_COMMANDSETS+1] << 16) | ident[ATA_IDENT_COMMANDSETS];
    if (cmd_sets & (1 << 10)) {
        /* LBA48 supported */
        dev->is_48bit = 1;
        dev->max_lba = ((uint64_t)ident[ATA_IDENT_MAX_LBA_EXT+1] << 16) | ident[ATA_IDENT_MAX_LBA_EXT];
    } else {
        dev->is_48bit = 0;
        dev->max_lba = ((uint32_t)ident[ATA_IDENT_MAX_LBA+1] << 16) | ident[ATA_IDENT_MAX_LBA];
    }
    dev->total_sectors = (uint32_t)dev->max_lba;

    return 1;
}

void ata_init(void) {
    device_count = 0;
    memset(devices, 0, sizeof(devices));

    /* Probe primary controller */
    outb(ATA_PRIMARY_CTRL, 0x06);  /* Disable interrupts */
    ata_delay(ATA_PRIMARY_CTRL);
    if (ata_identify(ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, 0, &devices[0]))
        device_count++;
    if (ata_identify(ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, 1, &devices[1]))
        device_count++;

    /* Probe secondary controller */
    outb(ATA_SECONDARY_CTRL, 0x06);
    ata_delay(ATA_SECONDARY_CTRL);
    if (ata_identify(ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, 0, &devices[2]))
        device_count++;
    if (ata_identify(ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, 1, &devices[3]))
        device_count++;
}

int ata_read_sectors(ata_device_t *dev, uint32_t lba, uint32_t count, void *buf) {
    if (!dev->present) return -1;

    uint16_t io = dev->io_base;
    uint8_t *ptr = (uint8_t*)buf;

    for (uint32_t s = 0; s < count; s++) {
        uint32_t sector = lba + s;

        if (dev->is_48bit) {
            /* 48-bit LBA */
            outb(io + ATA_REG_FEATURES, 0);
            outb(io + ATA_REG_SECCOUNT, (count > 1) ? count : 0);
            outb(io + ATA_REG_LBA_LO, sector & 0xFF);
            outb(io + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
            outb(io + ATA_REG_LBA_HI, (sector >> 16) & 0xFF);
            outb(io + ATA_REG_LBA_LO, (sector >> 24) & 0xFF);
            outb(io + ATA_REG_LBA_MID, 0);
            outb(io + ATA_REG_LBA_HI, 0);
            outb(io + ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);
        } else {
            /* 28-bit LBA */
            ata_select(io, dev->slave);
            ata_delay(dev->ctrl_base);
            outb(io + ATA_REG_SECCOUNT, 1);
            outb(io + ATA_REG_LBA_LO, sector & 0xFF);
            outb(io + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
            outb(io + ATA_REG_LBA_HI, (sector >> 16) & 0xFF);
            outb(io + ATA_REG_DRIVE, 0xE0 | (dev->slave << 4) | ((sector >> 24) & 0x0F));
            outb(io + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
        }

        if (ata_wait_ready(io) != 0) return -1;
        if (ata_wait_drq(io) != 0) return -1;

        for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++)
            ((uint16_t*)ptr)[i] = inw(io + ATA_REG_DATA);
        ptr += ATA_SECTOR_SIZE;
    }
    return count;
}

int ata_write_sectors(ata_device_t *dev, uint32_t lba, uint32_t count, const void *buf) {
    if (!dev->present) return -1;

    uint16_t io = dev->io_base;
    const uint8_t *ptr = (const uint8_t*)buf;

    for (uint32_t s = 0; s < count; s++) {
        uint32_t sector = lba + s;

        if (dev->is_48bit) {
            outb(io + ATA_REG_FEATURES, 0);
            outb(io + ATA_REG_SECCOUNT, (count > 1) ? count : 0);
            outb(io + ATA_REG_LBA_LO, sector & 0xFF);
            outb(io + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
            outb(io + ATA_REG_LBA_HI, (sector >> 16) & 0xFF);
            outb(io + ATA_REG_LBA_LO, (sector >> 24) & 0xFF);
            outb(io + ATA_REG_LBA_MID, 0);
            outb(io + ATA_REG_LBA_HI, 0);
            outb(io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO_EXT);
        } else {
            ata_select(io, dev->slave);
            ata_delay(dev->ctrl_base);
            outb(io + ATA_REG_SECCOUNT, 1);
            outb(io + ATA_REG_LBA_LO, sector & 0xFF);
            outb(io + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
            outb(io + ATA_REG_LBA_HI, (sector >> 16) & 0xFF);
            outb(io + ATA_REG_DRIVE, 0xE0 | (dev->slave << 4) | ((sector >> 24) & 0x0F));
            outb(io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
        }

        if (ata_wait_ready(io) != 0) return -1;
        if (ata_wait_drq(io) != 0) return -1;

        for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++)
            outw(io + ATA_REG_DATA, ((uint16_t*)ptr)[i]);
        ptr += ATA_SECTOR_SIZE;

        /* Flush */
        outb(io + ATA_REG_COMMAND, 0xE7);
        ata_wait_ready(io);
    }
    return count;
}

ata_device_t *ata_get_device(int index) {
    if (index < 0 || index >= 4 || !devices[index].present) return NULL;
    return &devices[index];
}

void ata_dump_info(ata_device_t *dev) {
    vga_puts("  Model:    "); vga_puts(dev->model); vga_putchar('\n');
    vga_puts("  Sectors:  "); vga_put_dec(dev->total_sectors); vga_putchar('\n');
    vga_puts("  Size:     "); vga_put_dec(dev->total_sectors / 2048); vga_puts(" MB\n");
    vga_puts("  LBA48:    "); vga_puts(dev->is_48bit ? "yes" : "no"); vga_putchar('\n');
    vga_puts("  CHS:      ");
    vga_put_dec(dev->cylinders); vga_puts("x");
    vga_put_dec(dev->heads); vga_puts("x");
    vga_put_dec(dev->sectors); vga_putchar('\n');
}
