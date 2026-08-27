; croOS gdt_flush.S - Load the GDT and reload segment registers
global gdt_flush

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    ; Reload kernel segments
    mov ax, 0x10       ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload CS
    jmp 0x08:.flush
.flush:
    ret
