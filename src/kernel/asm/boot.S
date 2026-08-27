# croOS boot.S - Multiboot header and entry point
# GAS syntax for i686
# Requests linear framebuffer for GUI mode

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FBINFO,   1<<2          # Request framebuffer
.set FLAGS,    ALIGN | MEMINFO | FBINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

# Multiboot header
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Multiboot framebuffer request (flag bit 2 set)
.long 0             # header_addr (0 = load from load addr)
.long 0             # load_addr (0 = loaded at _start)
.long 0             # load_end_addr (0 = whole file)
.long 0             # bss_end_addr (0 = no BSS)
.long 0             # entry_addr (0 = _start)

# Framebuffer fields
.long 0             # mode_type (0 = linear graphics)
.long 0             # width (0 = preferred)
.long 0             # height (0 = preferred)
.long 32            # bpp (32 bits per pixel)

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
    pushl %eax          # multiboot magic
    pushl %ebx          # multiboot info pointer
    call kmain
    cli
.hang:
    hlt
    jmp .hang
