/* croOS fat16.c - FAT16 filesystem implementation
 * Reads FAT16 filesystems from ATA disk, supports root directory,
 * subdirectories, file reading/writing, and FAT chain traversal. */

#include "kernel/types.h"
#include "fat16.h"
#include "drivers/ata.h"
#include "drivers/vga.h"
#include "mm/kmalloc.h"
#include "string.h"

static fat16_bpb_t bpb;
static uint32_t fat_start;
static uint32_t data_start;
static uint32_t root_dir_start;
static uint8_t *fat_cache = NULL;
static ata_device_t *disk = NULL;

/* Convert 8.3 filename to readable string */
static void fat16_name_to_string(const uint8_t *raw_name, char *out) {
    int pos = 0;
    for (int i = 0; i < 8; i++) {
        if (raw_name[i] == ' ') break;
        out[pos++] = raw_name[i] | 0x20;
    }
    if (raw_name[8] != ' ') {
        out[pos++] = '.';
        for (int i = 8; i < 11; i++) {
            if (raw_name[i] == ' ') break;
            out[pos++] = raw_name[i] | 0x20;
        }
    }
    out[pos] = '\0';
}

/* Convert string to 8.3 format */
static void fat16_string_to_name(const char *name, uint8_t *out) {
    memset(out, ' ', 11);
    int pos = 0;
    int i = 0;

    /* Name part */
    while (name[i] && name[i] != '.' && pos < 8) {
        out[pos++] = name[i] & ~0x20;
        i++;
    }

    /* Extension part */
    if (name[i] == '.') {
        i++;
        pos = 8;
        while (name[i] && pos < 11) {
            out[pos++] = name[i] & ~0x20;
            i++;
        }
    }
}

/* Read a FAT16 entry to get next cluster */
static uint16_t fat16_get_next_cluster(uint16_t cluster) {
    if (!fat_cache) return 0xFFFF;
    uint32_t offset = cluster * 2;
    return *(uint16_t*)&fat_cache[offset];
}

/* Read a sector from disk */
static int fat16_read_sector(uint32_t lba, void *buf) {
    if (!disk) return -1;
    return ata_read_sectors(disk, lba, 1, buf);
}

/* Read a cluster chain */
static int fat16_read_cluster_chain(uint16_t first_cluster, void *buf, uint32_t max_bytes) {
    uint8_t *ptr = (uint8_t*)buf;
    uint32_t bytes_read = 0;
    uint16_t cluster = first_cluster;
    uint8_t *sector_buf = kmalloc(FAT16_SECTOR_SIZE);
    if (!sector_buf) return -1;

    while (cluster >= 2 && cluster < 0xFFF8 && bytes_read < max_bytes) {
        uint32_t lba = data_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;

        for (int s = 0; s < bpb.sectors_per_cluster && bytes_read < max_bytes; s++) {
            if (fat16_read_sector(lba + s, sector_buf) != 1) {
                kfree(sector_buf);
                return -1;
            }
            uint32_t to_copy = max_bytes - bytes_read;
            if (to_copy > FAT16_SECTOR_SIZE) to_copy = FAT16_SECTOR_SIZE;
            memcpy(ptr + bytes_read, sector_buf, to_copy);
            bytes_read += to_copy;
        }

        cluster = fat16_get_next_cluster(cluster);
    }

    kfree(sector_buf);
    return (int)bytes_read;
}

/* Find a directory entry in root or subdirectory */
static int fat16_find_entry(uint16_t dir_cluster, const char *name, fat16_dir_entry_t *entry) {
    uint8_t dir_buf[4096];
    uint16_t target[11];
    fat16_string_to_name(name, (uint8_t*)target);

    uint16_t cluster = dir_cluster;
    while (cluster >= 2 && cluster < 0xFFF8) {
        uint32_t lba = data_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
        uint8_t *sector_buf = kmalloc(FAT16_SECTOR_SIZE);
        if (!sector_buf) return -1;

        for (int s = 0; s < bpb.sectors_per_cluster; s++) {
            if (fat16_read_sector(lba + s, sector_buf) != 1) {
                kfree(sector_buf);
                return -1;
            }
            int entries_per_sector = FAT16_SECTOR_SIZE / 32;
            for (int e = 0; e < entries_per_sector; e++) {
                fat16_dir_entry_t *de = (fat16_dir_entry_t*)(sector_buf + e * 32);
                if (de->name[0] == 0x00) { kfree(sector_buf); return -1; } /* end */
                if (de->name[0] == 0xE5) continue; /* deleted */
                if (de->attr & FAT16_ATTR_LONG_NAME) continue;
                if (de->attr & FAT16_ATTR_VOLUME_ID) continue;

                if (memcmp(de->name, target, 11) == 0) {
                    memcpy(entry, de, sizeof(fat16_dir_entry_t));
                    kfree(sector_buf);
                    return 0;
                }
            }
            kfree(sector_buf);
        }
        cluster = fat16_get_next_cluster(cluster);
    }
    return -1;
}

/* Initialize FAT16 from an ATA device */
void fat16_init(void) {
    disk = NULL;

    /* Find first ATA device */
    for (int i = 0; i < 4; i++) {
        ata_device_t *dev = ata_get_device(i);
        if (dev && dev->present) {
            disk = dev;
            break;
        }
    }
    if (!disk) return;

    /* Read boot sector */
    uint8_t *boot = kmalloc(FAT16_SECTOR_SIZE);
    if (!boot) return;

    if (fat16_read_sector(0, boot) != 1) {
        kfree(boot);
        return;
    }

    memcpy(&bpb, boot, sizeof(fat16_bpb_t));
    kfree(boot);

    /* Validate FAT16 */
    if (bpb.bytes_per_sector != FAT16_SECTOR_SIZE) return;
    if (bpb.fat_size_16 == 0) return; /* Not FAT16 */

    /* Calculate layout */
    fat_start = bpb.reserved_sectors;
    root_dir_start = fat_start + bpb.num_fats * bpb.fat_size_16;
    data_start = root_dir_start + (bpb.root_entry_count * 32 + FAT16_SECTOR_SIZE - 1) / FAT16_SECTOR_SIZE;

    /* Load FAT into cache */
    uint32_t fat_size_bytes = bpb.fat_size_16 * FAT16_SECTOR_SIZE;
    fat_cache = kmalloc(fat_size_bytes);
    if (fat_cache) {
        for (uint32_t s = 0; s < bpb.fat_size_16; s++) {
            fat16_read_sector(fat_start + s, fat_cache + s * FAT16_SECTOR_SIZE);
        }
    }
}

void fat16_register(vfs_fs_t *vfs) {
    (void)vfs;
    /* FAT16 can be mounted via: vfs_mount("/fat", &fat16_vfs) */
}

int fat16_read_file(const char *path, void *buf, uint32_t max_size) {
    fat16_dir_entry_t entry;
    if (fat16_find_entry(0, path, &entry) < 0) return -1;
    if (entry.attr & FAT16_ATTR_DIRECTORY) return -1;

    return fat16_read_cluster_chain(entry.first_cluster, buf, max_size < entry.file_size ? max_size : entry.file_size);
}

int fat16_write_file(const char *path, const void *buf, uint32_t size) {
    (void)path; (void)buf; (void)size;
    /* TODO: allocate cluster chain and write */
    return -1;
}

int fat16_list_dir(const char *path, char *out_buf, uint32_t buf_size) {
    uint16_t dir_cluster = 0;  /* Root directory */
    if (path && path[0]) {
        fat16_dir_entry_t entry;
        if (fat16_find_entry(0, path, &entry) == 0)
            dir_cluster = entry.first_cluster;
        else return -1;
    }

    int pos = 0;
    uint16_t cluster = dir_cluster;
    while (cluster >= 2 && cluster < 0xFFF8 && (uint32_t)pos < buf_size - 64) {
        uint32_t lba = data_start + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
        uint8_t *sector_buf = kmalloc(FAT16_SECTOR_SIZE);
        if (!sector_buf) break;

        for (int s = 0; s < bpb.sectors_per_cluster; s++) {
            if (fat16_read_sector(lba + s, sector_buf) != 1) {
                kfree(sector_buf);
                return pos;
            }
            int entries_per_sector = FAT16_SECTOR_SIZE / 32;
            for (int e = 0; e < entries_per_sector; e++) {
                fat16_dir_entry_t *de = (fat16_dir_entry_t*)(sector_buf + e * 32);
                if (de->name[0] == 0x00) { kfree(sector_buf); return pos; }
                if (de->name[0] == 0xE5) continue;
                if (de->attr & FAT16_ATTR_LONG_NAME) continue;
                if (de->attr & FAT16_ATTR_VOLUME_ID) continue;

                char name[13];
                fat16_name_to_string(de->name, name);
                strcpy(out_buf + pos, name);
                if (de->attr & FAT16_ATTR_DIRECTORY) strcat(out_buf + pos, "/");
                strcat(out_buf + pos, "\n");
                pos += strlen(out_buf + pos);
            }
        }
        kfree(sector_buf);
        cluster = fat16_get_next_cluster(cluster);
    }
    return pos;
}
