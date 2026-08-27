/* croOS elf.h - ELF binary loader */
#ifndef _ELF_H
#define _ELF_H

#include "kernel/types.h"

#define EI_NIDENT 16

typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __packed elf32_ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __packed elf32_phdr;

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

int      elf_validate(const void *data, uint32_t size);
uint32_t elf_get_entry(const void *data);
int      elf_load(const void *data, uint32_t size, uint32_t *entry_out);
int      elf_is_executable(const void *data);
uint32_t elf_get_phdr_count(const void *data);

#endif
