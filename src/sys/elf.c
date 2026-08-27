/* croOS elf.c - ELF binary loader
 * Validates ELF headers, loads PT_LOAD segments into memory,
 * sets up page tables, and returns the entry point address. */

#include "kernel/types.h"
#include "elf.h"
#include "mm/kmalloc.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "drivers/vga.h"
#include "string.h"

static int elf_check_magic(const elf32_ehdr *hdr) {
    return hdr->e_ident[0] == 0x7F &&
           hdr->e_ident[1] == 'E' &&
           hdr->e_ident[2] == 'L' &&
           hdr->e_ident[3] == 'F';
}

int elf_validate(const void *data, uint32_t size) {
    if (size < sizeof(elf32_ehdr)) return -1;
    const elf32_ehdr *hdr = (const elf32_ehdr*)data;

    if (!elf_check_magic(hdr)) return -1;
    if (hdr->e_ident[4] != 1) return -1;  /* 32-bit only */
    if (hdr->e_ident[5] != 1) return -1;  /* Little-endian only */
    if (hdr->e_type != 2 && hdr->e_type != 3) return -1;  /* ET_EXEC or ET_DYN */
    if (hdr->e_machine != 3) return -1;  /* EM_386 */
    if (hdr->e_version != 1) return -1;
    if (hdr->e_ehsize < sizeof(elf32_ehdr)) return -1;
    if (hdr->e_phoff + hdr->e_phnum * hdr->e_phentsize > size) return -1;

    return 0;
}

uint32_t elf_get_entry(const void *data) {
    return ((const elf32_ehdr*)data)->e_entry;
}

int elf_is_executable(const void *data) {
    const elf32_ehdr *hdr = (const elf32_ehdr*)data;
    return hdr->e_type == 2;  /* ET_EXEC */
}

uint32_t elf_get_phdr_count(const void *data) {
    return ((const elf32_ehdr*)data)->e_phnum;
}

int elf_load(const void *data, uint32_t size, uint32_t *entry_out) {
    if (elf_validate(data, size) < 0) {
        vga_puts("  [ELF] Invalid ELF binary\n");
        return -1;
    }

    const elf32_ehdr *hdr = (const elf32_ehdr*)data;
    *entry_out = hdr->e_entry;

    vga_puts("  [ELF] Loading ");
    vga_puts(elf_is_executable(data) ? "executable" : "shared object");
    vga_puts(" (entry=");
    vga_put_hex(hdr->e_entry);
    vga_puts(", segments=");
    vga_put_dec(hdr->e_phnum);
    vga_puts(")\n");

    /* Load each PT_LOAD segment */
    const elf32_phdr *phdr = (const elf32_phdr*)(data + hdr->e_phoff);

    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;
        if (phdr[i].p_memsz == 0) continue;

        uint32_t vaddr = phdr[i].p_vaddr;
        uint32_t filesz = phdr[i].p_filesz;
        uint32_t memsz = phdr[i].p_memsz;

        vga_puts("  [ELF] Segment ");
        vga_put_dec(i);
        vga_puts(": vaddr=");
        vga_put_hex(vaddr);
        vga_puts(" filesz=");
        vga_put_hex(filesz);
        vga_puts(" memsz=");
        vga_put_hex(memsz);
        vga_puts(" flags=");
        vga_put_hex(phdr[i].p_flags);
        vga_putchar('\n');

        /* Calculate pages needed */
        uint32_t start_page = vaddr & ~(PAGE_SIZE - 1);
        uint32_t end_page = (vaddr + memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        uint32_t num_pages = (end_page - start_page) / PAGE_SIZE;

        /* Allocate and map pages */
        for (uint32_t p = 0; p < num_pages; p++) {
            void *page = pmm_alloc_page();
            if (!page) {
                vga_puts("  [ELF] Out of memory loading segment\n");
                return -1;
            }
            memset(page, 0, PAGE_SIZE);

            uint32_t flags = PAGE_PRESENT | PAGE_USER;
            if (phdr[i].p_flags & PF_W) flags |= PAGE_WRITE;

            /* Map at vaddr (simplified: identity map for now) */
            vmm_map_page(0, start_page + p * PAGE_SIZE, (uint32_t)page, flags);
        }

        /* Copy segment data */
        uint8_t *dest = (uint8_t*)vaddr;
        if (filesz > 0 && phdr[i].p_offset + filesz <= size) {
            memcpy(dest, data + phdr[i].p_offset, filesz);
        }
        /* Zero remaining (BSS) */
        if (memsz > filesz) {
            memset(dest + filesz, 0, memsz - filesz);
        }
    }

    return 0;
}
