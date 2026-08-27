# croOS gdt_flush.S - Load the GDT and reload segment registers
.global gdt_flush

gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)
    mov $0x10, %ax       # kernel data segment
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ljmp $0x08, $.flush
.flush:
    ret
