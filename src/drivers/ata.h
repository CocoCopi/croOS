/* croOS ata.h - ATA/IDE disk driver */
#ifndef _ATA_H
#define _ATA_H

#include "kernel/types.h"

/* Primary ATA I/O ports */
#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_IO    0x170
#define ATA_SECONDARY_CTRL  0x376

/* ATA registers */
#define ATA_REG_DATA        0
#define ATA_REG_ERROR       1
#define ATA_REG_FEATURES    1
#define ATA_REG_SECCOUNT    2
#define ATA_REG_LBA_LO      3
#define ATA_REG_LBA_MID     4
#define ATA_REG_LBA_HI      5
#define ATA_REG_DRIVE       6
#define ATA_REG_STATUS      7
#define ATA_REG_COMMAND     7

/* ATA status bits */
#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DRQ     0x08
#define ATA_SR_ERR     0x01

/* ATA commands */
#define ATA_CMD_READ_PIO      0x20
#define ATA_CMD_READ_PIO_EXT  0x24
#define ATA_CMD_WRITE_PIO     0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_IDENTIFY_PACKET 0xA1

/* ATA identification fields */
#define ATA_IDENT_MODEL       54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID  106
#define ATA_IDENT_MAX_LBA     120
#define ATA_IDENT_COMMANDSETS 164
#define ATA_IDENT_MAX_LBA_EXT 200

#define ATA_SECTOR_SIZE 512

typedef struct {
    uint16_t io_base;
    uint16_t ctrl_base;
    uint8_t  slave;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;
    uint32_t total_sectors;
    uint64_t max_lba;
    char     model[42];
    uint8_t  present;
    uint8_t  is_48bit;
} ata_device_t;

void ata_init(void);
int  ata_read_sectors(ata_device_t *dev, uint32_t lba, uint32_t count, void *buf);
int  ata_write_sectors(ata_device_t *dev, uint32_t lba, uint32_t count, const void *buf);
ata_device_t *ata_get_device(int index);
void ata_dump_info(ata_device_t *dev);

#endif
