#!/bin/bash
# croOS floppy disk builder - creates a 1.44MB FAT floppy with GRUB
set -e

BUILD_DIR="build"
FLOPPY="${BUILD_DIR}/croOS.flp"
GRUB_DIR="${BUILD_DIR}/grub_floppy"
KERNEL="${BUILD_DIR}/croOS.elf"

echo "=== Building croOS floppy image ==="

mkdir -p "${GRUB_DIR}/boot/grub"

# Copy kernel
cp "${KERNEL}" "${GRUB_DIR}/boot/croOS.elf"

# GRUB config
cat > "${GRUB_DIR}/boot/grub/grub.cfg" << 'GRUBEOF'
set timeout=3
set default=0

menuentry "croOS" {
    multiboot /boot/croOS.elf
    boot
}
GRUBEOF

# Create 1.44MB blank floppy image
dd if=/dev/zero of="${FLOPPY}" bs=1474560 count=1 2>/dev/null

# Install GRUB core image for floppy
grub-mkimage \
    -O i386-pc \
    -o "${GRUB_DIR}/boot/grub/core.img" \
    -p '(hd0)/boot/grub' \
    biosdisk fat part_msdos multiboot

# Create FAT12 filesystem on floppy
mkfs.fat -F 12 "${FLOPPY}" 2>/dev/null

# Copy files to floppy
mcopy -i "${FLOPPY}" "${GRUB_DIR}/boot" ::/boot
mcopy -i "${FLOPPY}" "${GRUB_DIR}/boot/grub/core.img" ::/boot/grub/core.img

# Install GRUB MBR boot sector
# Create a pre-built stage1 that works with multiboot
printf '\xEB\x5A\x90' | dd of="${FLOPPY}" bs=1 count=3 conv=notrunc 2>/dev/null
printf 'GRUB  ' | dd of="${FLOPPY}" bs=1 count=6 seek=3 conv=notrunc 2>/dev/null

# Use grub-bios-setup to write the boot sector
grub-bios-setup \
    --directory="${GRUB_DIR}/boot/grub" \
    "${FLOPPY}" 2>/dev/null || {
    echo "grub-bios-setup failed, using dd for MBR"
    # Alternative: write GRUB MBR manually
    dd if="${GRUB_DIR}/boot/grub/core.img" of="${FLOPPY}" bs=512 seek=1 conv=notrunc 2>/dev/null
}

echo ""
echo "=== Floppy image built ==="
echo "  Output: ${FLOPPY}"
ls -la "${FLOPPY}"
echo ""
echo "  Run with:"
echo "    qemu-system-i386 -fda ${FLOPPY}"
echo "    limbo -fda ${FLOPPY}"
