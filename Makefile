# croOS Makefile - Builds the full kernel
# Usage: make / make clean / make run / make debug

CC = i686-linux-gnu-gcc
AS = i686-linux-gnu-gcc
LD = i686-linux-gnu-ld

CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector \
         -nostdlib -nostdinc -Wall -Wextra \
         -Iinclude -Isrc -Isrc/kernel -c
ASFLAGS = -m32 -ffreestanding -c
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

BUILD = build
SRC = src

AS_SRCS = $(SRC)/kernel/asm/boot.S \
          $(SRC)/kernel/asm/isr.S \
          $(SRC)/kernel/asm/gdt_flush.S \
          $(SRC)/kernel/asm/vmm.S

C_SRCS = $(SRC)/kernel/kmain.c \
         $(SRC)/kernel/gdt.c \
         $(SRC)/kernel/idt.c \
         $(SRC)/kernel/process.c \
         $(SRC)/kernel/window.c \
         $(SRC)/mm/pmm.c \
         $(SRC)/mm/kmalloc.c \
         $(SRC)/mm/vmm.c \
         $(SRC)/drivers/vga.c \
         $(SRC)/drivers/keyboard.c \
         $(SRC)/drivers/timer.c \
         $(SRC)/drivers/serial.c \
         $(SRC)/drivers/pci.c \
         $(SRC)/drivers/ata.c \
         $(SRC)/drivers/mouse.c \
         $(SRC)/drivers/audio.c \
         $(SRC)/drivers/usb.c \
         $(SRC)/fs/vfs.c \
         $(SRC)/fs/ramdisk.c \
         $(SRC)/fs/fat16.c \
         $(SRC)/fs/proc.c \
         $(SRC)/net/net.c \
         $(SRC)/net/dhcp.c \
         $(SRC)/net/dns.c \
         $(SRC)/net/http.c \
         $(SRC)/sys/syscall.c \
         $(SRC)/sys/pipe.c \
         $(SRC)/sys/signal.c \
         $(SRC)/sys/elf.c \
         $(SRC)/libc/string.c \
         $(SRC)/libc/math.c \
         $(SRC)/libc/stdio.c \
         $(SRC)/libc/stdlib.c

OBJS = $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(C_SRCS)) \
       $(patsubst $(SRC)/%.S,$(BUILD)/%.o,$(AS_SRCS))

TARGET = $(BUILD)/croOS.elf

.PHONY: all clean run debug

all: $(TARGET)
	@echo ""
	@echo "============================================"
	@echo "  croOS kernel built successfully!"
	@echo "  Output: $(TARGET)"
	@stat -c "  Size:   %s bytes" $(TARGET) 2>/dev/null || true
	@echo "============================================"

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/%.o: $(SRC)/%.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD)

run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -serial stdio -display stdio

iso: $(TARGET)
	@mkdir -p iso/boot/grub
	@cp $(TARGET) iso/boot/croOS.elf
	@echo 'set timeout=5' > iso/boot/grub/grub.cfg
	@echo 'set default=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "croOS 3.0" {' >> iso/boot/grub/grub.cfg
	@echo '    multiboot /boot/croOS.elf' >> iso/boot/grub/grub.cfg
	@echo '    boot' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/croOS.iso iso/
	@echo ''
	@echo '  ISO: $(BUILD)/croOS.iso'
	@stat -c '  Size: %s bytes' $(BUILD)/croOS.iso

diso: $(TARGET)
	qemu-system-i386 -cdrom $(BUILD)/croOS.iso -serial stdio -display stdio

debug: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -serial stdio -display stdio -s -S
