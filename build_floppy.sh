#!/bin/bash
# croOS Floppy Image Builder
# Creates a bootable floppy image with:
# - Real-mode bootloader (sets VESA mode via BIOS)
# - croOS kernel (loaded by bootloader to 1MB)
#
# Usage:
#   chmod +x build_floppy.sh
#   ./build_floppy.sh

set -e

echo "============================================"
echo "  croOS Floppy Image Builder"
echo "============================================"

# Step 1: Build the kernel ELF
echo "[1/5] Building kernel..."
make clean
make -j$(nproc 2>/dev/null || echo 4)

if [ ! -f build/croOS.elf ]; then
    echo "ERROR: Kernel build failed"
    exit 1
fi
echo "  Kernel built: $(stat -c%s build/croOS.elf) bytes"

# Step 2: Extract flat binary from ELF (needed for floppy loading)
echo "[2/5] Extracting flat binary..."
i686-linux-gnu-objcopy -O binary build/croOS.elf build/croOS.bin
echo "  Flat binary: $(stat -c%s build/croOS.bin) bytes"

# Check binary size (must fit in floppy after boot sector)
BOOT_SIZE=512
MAX_SIZE=$((1474560 - BOOT_SIZE))  # 1.44MB floppy minus boot sector
BIN_SIZE=$(stat -c%s build/croOS.bin)
echo "  Available space: $MAX_SIZE bytes"
echo "  Kernel size: $BIN_SIZE bytes"

if [ "$BIN_SIZE" -gt "$MAX_SIZE" ]; then
    echo "WARNING: Kernel too large for floppy! Truncating..."
    truncate -s $MAX_SIZE build/croOS.bin
fi

# Step 3: Assemble bootloader
echo "[3/5] Assembling bootloader..."
nasm -f bin bootloader/boot.asm -o build/boot.bin
echo "  Bootloader built: $(stat -c%s build/boot.bin) bytes"

if [ "$(stat -c%s build/boot.bin)" -ne 512 ]; then
    echo "ERROR: Bootloader must be exactly 512 bytes!"
    exit 1
fi

# Step 4: Create 1.44MB floppy image
echo "[4/5] Creating floppy image..."
dd if=/dev/zero of=build/croOS.img bs=512 count=2880 2>/dev/null

# Write boot sector (first 512 bytes)
dd if=build/boot.bin of=build/croOS.img bs=512 count=1 conv=notrunc 2>/dev/null

# Write kernel starting at sector 1 (byte 512)
dd if=build/croOS.bin of=build/croOS.img bs=512 seek=1 conv=notrunc 2>/dev/null
echo "  Floppy image: $(stat -c%s build/croOS.img) bytes (1.44MB)"

# Step 5: Build GRUB ISO as backup (for GRUB boot)
echo "[5/5] Building GRUB ISO..."
mkdir -p iso/boot
cp build/croOS.elf iso/boot/croOS.elf
mkdir -p iso/boot/grub
cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=3
set default=0
insmod all_video
menuentry "croOS 4.0 HyperCorros" {
    insmod multiboot
    multiboot /boot/croOS.elf
    boot
}
EOF
grub-mkrescue --modules="multiboot normal boot" -o build/croOS.iso iso/ 2>/dev/null || true

echo ""
echo "============================================"
echo "  Build complete!"
echo "============================================"
echo ""
echo "  Boot from floppy (recommended for GUI):"
echo "    qemu-system-i386 -fda build/croOS.img -m 256 -serial stdio"
echo ""
echo "  Boot from ISO (GRUB):"
echo "    qemu-system-i386 -cdrom build/croOS.iso -m 256 -serial stdio -boot d"
echo ""
echo "  Direct kernel (no GUI, text mode only):"
echo "    qemu-system-i386 -kernel build/croOS.elf -m 256 -serial stdio"
echo ""
