; croOS Bootloader - Compact Real Mode Boot Sector
; Sets VESA mode, loads kernel, enters protected mode
; Must fit in exactly 512 bytes (boot sector)

[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [drive], dl

    ; Clear framebuffer info at 0x500 (32 bytes)
    mov di, 0x500
    mov cx, 16
    xor ax, ax
    rep stosw

    ; Set VESA 1024x768x32 (mode 0x4105)
    mov ax, 0x4F02
    mov bx, 0x4105
    int 0x10

    ; Get mode info to 0x8000
    mov ax, 0x4F01
    mov cx, 0x4105
    mov di, 0x8000
    int 0x10

    ; Save framebuffer info at 0x500
    ; offset 0x28=PhysBasePtr, 0x12=Pitch, 0x14=W, 0x16=H, 0x1B=BPP
    lea si, [0x8028]       ; PhysBasePtr
    mov di, 0x500
    movsw                  ; addr low word
    movsw                  ; addr high word
    lea si, [0x8012]
    movsw                  ; pitch
    movsw                  ; width
    movsw                  ; height
    lodsb                  ; skip XCharSize
    lodsb                  ; skip YCharSize
    lodsb                  ; skip NumPlanes
    lodsb                  ; BPP
    mov [0x510], al
    mov byte [0x514], 1    ; available = true

    ; Load kernel from floppy to 0x10000 using CHS
    ; Sector 1 onwards, 128KB = 256 sectors
    ; Load in chunks of 32 sectors (to avoid track boundary issues)
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000         ; ES:BX = 0x1000:0x0000 = phys 0x10000
    mov ch, 0              ; cylinder 0
    mov cl, 2              ; start sector 2 (sector 1 = this bootloader)
    mov dh, 0              ; head 0
    mov dl, [drive]

    ; Load 4 chunks of 32 sectors = 128KB
    mov bp, 4              ; chunk counter
    mov si, 0              ; sector offset

.load_loop:
    push bp
    mov ah, 0x02           ; read sectors
    mov al, 32             ; 32 sectors = 16KB per chunk
    int 0x13
    jc .load_done

    ; Advance: add 32 to sector, handle track boundaries
    add si, 32
    add bx, 32*512         ; advance buffer
    cmp si, 18             ; sectors per track
    jb .load_loop_next
    ; Crossed track boundary - advance head/cylinder
    sub si, 18
    inc dh                  ; next head
    and dh, 1
    jnz .load_loop_next
    inc ch                  ; next cylinder
.load_loop_next:
    pop bp
    dec bp
    jnz .load_loop

.load_done:
    pop bp                  ; balance stack

    ; Create multiboot info at 0x800
    ; flags: bit0=mem, bit12=framebuffer
    mov dword [0x800], (1<<0)|(1<<12)
    mov dword [0x804], 640        ; mem_lower
    mov dword [0x808], 261120     ; mem_upper (256MB)

    ; Copy framebuffer info from 0x500 to multiboot at 0x800+88
    mov esi, 0x500
    mov edi, 0x858               ; 0x800 + 88
    mov ecx, 4                   ; 16 bytes (addr, pitch, width, height)
    rep movsd
    mov al, [0x510]
    mov [0x868], al              ; bpp
    mov byte [0x869], 0          ; type=RGB

    ; Enable A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Switch to protected mode
    cli
    lgdt [gdtr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm

; ---- Data ----
drive: db 0

gdtr:
    dw gdt_end - gdt - 1
    dd gdt

gdt:
    dd 0, 0                    ; null
    dw 0xFFFF, 0, 0, 0x9A, 0xCF, 0  ; code
    dw 0xFFFF, 0, 0, 0x92, 0xCF, 0  ; data
gdt_end:

; ---- 32-bit code ----
[BITS 32]
pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; Move kernel from 0x10000 to 0x100000 (1MB)
    cld
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, 32768             ; 128KB / 4 = 32768 dwords
    rep movsd

    ; Multiboot state
    mov ebx, 0x800
    mov eax, 0x2BADB002
    jmp 0x100000

times 510-($-$$) db 0
dw 0xAA55
