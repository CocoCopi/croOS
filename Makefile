# croOS Makefile — Builds the full kernel
# Usage: make          (build kernel)
#        make clean    (clean build artifacts)
#        make run      (run in QEMU)
#        make debug    (run in QEMU with GDB stub)

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

# Assembly sources (GAS syntax)
AS_SRCS = $(SRC)/kernel/asm/boot.S \
          $(SRC)/kernel/asm/isr.S \
          $(SRC)/kernel/asm/gdt_flush.S \
          $(SRC)/kernel/asm/vmm.S

# C sources
C_SRCS = $(SRC)/kernel/kmain.c \
         $(SRC)/kernel/gdt.c \
         $(SRC)/kernel/idt.c \
         $(SRC)/kernel/process.c \
         $(SRC)/mm/pmm.c \
         $(SRC)/mm/kmalloc.c \
         $(SRC)/mm/vmm.c \
         $(SRC)/drivers/vga.c \
         $(SRC)/drivers/keyboard.c \
         $(SRC)/drivers/timer.c \
         $(SRC)/drivers/serial.c \
         $(SRC)/fs/vfs.c \
         $(SRC)/fs/ramdisk.c \
         $(SRC)/net/net.c \
         $(SRC)/libc/string.c \
         $(SRC)/sys/syscall.c

OBJS = $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(C_SRCS)) \
       $(patsubst $(SRC)/%.S,$(BUILD)/%.o,$(AS_SRCS))

TARGET = $(BUILD)/croOS.elf

.PHONY: all clean run debug

all: $(TARGET)
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  croOS kernel built successfully!"
	@echo "  Output: $(TARGET)"
	@stat -c "  Size:   %s bytes" $(TARGET) 2>/dev/null || true
	@echo "═══════════════════════════════════════════"

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

debug: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -serial stdio -display stdio -s -S
