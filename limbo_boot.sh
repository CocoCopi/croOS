#!/bin/bash
# croOS boot script for Limbo Emulator
# Usage: ./limbo_boot.sh
# Or in Limbo: load kernel=build/croOS.elf, ram=256

echo "=== croOS for Limbo Emulator ==="
echo ""
echo "Method 1: Direct kernel (recommended for Limbo)"
echo "  1. Open Limbo Emulator"
echo "  2. Set kernel to: $(pwd)/build/croOS.elf"
echo "  3. Set RAM to 256 MB"
echo "  4. Set CPU to: 486/vx"
echo "  5. Enable: VNC Display"
echo "  6. Boot!"
echo ""
echo "Method 2: ISO image"
echo "  1. Open Limbo Emulator"
echo "  2. Set CD-ROM to: $(pwd)/build/croOS.iso"
echo "  3. Set boot from CD-ROM"
echo "  4. Set RAM to 256 MB"
echo "  5. Boot!"
echo ""
echo "Method 3: QEMU (if available)"
echo "  qemu-system-i386 -kernel build/croOS.elf -m 256 -serial stdio"
echo "  qemu-system-i386 -cdrom build/croOS.iso -m 256"
