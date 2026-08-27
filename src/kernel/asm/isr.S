# croOS isr.S - ISR and IRQ stubs (256 entries)
# Each stub pushes error code (or dummy 0) + interrupt number

.extern isr_handler
.extern irq_handler

.macro ISR_NOERR num
.global isr\num
isr\num:
    pushl $0
    pushl $\num
    jmp isr_common
.endm

.macro ISR_ERR num
.global isr\num
isr\num:
    pushl $\num
    jmp isr_common
.endm

.macro IRQ num, intnum
.global irq\num
irq\num:
    pushl $0
    pushl $\intnum
    jmp irq_common
.endm

# ISRs 0-31
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

# IRQs 0-15 (INT 32-47)
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

# Common ISR handler
isr_common:
    pusha
    mov %ds, %ax
    push %eax
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp
    call isr_handler
    add $4, %esp
    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    popa
    add $8, %esp
    iret

# Common IRQ handler
irq_common:
    pusha
    mov %ds, %ax
    push %eax
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp
    call irq_handler
    add $4, %esp
    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    popa
    add $8, %esp
    iret

# IDT flush
.global idt_flush
idt_flush:
    mov 4(%esp), %eax
    lidt (%eax)
    sti
    ret
