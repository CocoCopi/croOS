# croOS boot.S - Multiboot header and entry point
# GAS syntax for i686

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

# Multiboot header
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Stack (16KB)
.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

# Entry point
.section .text
.global _start
.extern kmain

_start:
    movl $stack_top, %esp
    pushl %eax          # magic
    pushl %ebx          # mbi pointer
    call kmain
    cli
.hang:
    hlt
    jmp .hang
