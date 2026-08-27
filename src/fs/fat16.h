/* croOS fat16.h - FAT16 filesystem implementation */
#ifndef _FAT16_H
#define _FAT16_H

#include "vfs.h"
#include "kernel/types.h"

#define FAT16_SECTOR_SIZE   512
#define FAT16_MAX_PATH      256
#define FAT16_MAX_OPEN      32

/* FAT16 BIOS Parameter Block */
typedef struct __packed {
    uint8_t  jmp_boot[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t  drive_number;
    uint8_t  reserved;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} fat16_bpb_t;

/* FAT16 directory entry */
typedef struct __packed {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_hi;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t first_cluster;
    uint32_t file_size;
} fat16_dir_entry_t;

#define FAT16_ATTR_READ_ONLY  0x01
#define FAT16_ATTR_HIDDEN     0x02
#define FAT16_ATTR_SYSTEM     0x04
#define FAT16_ATTR_VOLUME_ID  0x08
#define FAT16_ATTR_DIRECTORY  0x10
#define FAT16_ATTR_ARCHIVE    0x20
#define FAT16_ATTR_LONG_NAME  0x0F

void fat16_init(void);
void fat16_register(vfs_fs_t *vfs);
int  fat16_read_file(const char *path, void *buf, uint32_t max_size);
int  fat16_write_file(const char *path, const void *buf, uint32_t size);
int  fat16_list_dir(const char *path, char *out_buf, uint32_t buf_size);

#endif
