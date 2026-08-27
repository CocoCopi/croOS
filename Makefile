# croOS Makefile - Builds the full kernel
# Auto-detects cross-compiler or falls back to native GCC

# Auto-detect CC
CC := $(shell which i686-linux-gnu-gcc 2>/dev/null || which i686-elf-gcc 2>/dev/null || echo "")
LD := $(shell which i686-linux-gnu-ld 2>/dev/null || which i686-elf-ld 2>/dev/null || echo "")

# If no cross-compiler found, fail with helpful message
ifeq ($(CC),)
  $(error ============================================\
    croOS requires an i686 cross-compiler.          \
    Build inside proot-distro:                       \
      proot-distro login ubuntu                      \
      apt-get install -y gcc-i686-linux-gnu          \
      cd /sdcard/Projects/croOS && make              \
    Then run from Termux:                            \
      qemu-system-i386 -kernel build/croOS.elf -m 256 -serial stdio\
    ============================================)
endif

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

.PHONY: all clean run iso help

all: $(TARGET)
	@echo ""
	@echo "============================================"
	@echo "  croOS kernel built successfully!"
	@echo "  Output: $(TARGET)"
	@stat -c "  Size:   %s bytes" $(TARGET) 2>/dev/null || true
	@echo "  CC: $(CC)"
	@echo "============================================"
	@echo ""
	@echo "  To run: qemu-system-i386 -kernel $(TARGET) -m 256 -serial stdio"
	@echo "  To make ISO: make iso"

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/%.o: $(SRC)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD)

run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -m 256 -serial stdio

iso: $(TARGET)
	@mkdir -p iso/boot/grub
	@cp $(TARGET) iso/boot/croOS.elf
	@echo 'set timeout=5' > iso/boot/grub/grub.cfg
	@echo 'set default=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "croOS 4.0" {' >> iso/boot/grub/grub.cfg
	@echo '    multiboot /boot/croOS.elf' >> iso/boot/grub/grub.cfg
	@echo '    boot' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --modules="multiboot normal boot" -o $(BUILD)/croOS.iso iso/
	@echo ""
	@echo "  ISO built: $(BUILD)/croOS.iso"
	@echo "  To run: qemu-system-i386 -cdrom $(BUILD)/croOS.iso -m 256 -serial stdio"

help:
	@echo "croOS build system"
	@echo ""
	@echo "Usage (must be inside proot-distro):"
	@echo "  make          Build croOS kernel"
	@echo "  make clean    Remove build artifacts"
	@echo "  make run      Build and run in QEMU"
	@echo "  make iso      Build bootable ISO for Limbo/QEMU"
	@echo "  make help     Show this help"
	@echo ""
	@echo "Quick start:"
	@echo "  proot-distro login ubuntu"
	@echo "  cd /sdcard/Projects/croOS"
	@echo "  apt-get install -y gcc-i686-linux-gnu grub-pc-bin grub-common xorriso"
	@echo "  make clean && make"
	@echo "  make iso"
	@echo "  exit"
	@echo "  qemu-system-i386 -kernel build/croOS.elf -m 256 -serial stdio"
	@echo "  qemu-system-i386 -cdrom build/croOS.iso -m 256"
