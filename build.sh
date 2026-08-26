#!/bin/bash
# corrOS build script — compiles the Corros kernel to a freestanding ELF
set -e
cd "$(dirname "$0")"
mkdir -p build

CORROS="${CORROS:-corros}"
CC="i686-linux-gnu-gcc"
LD="i686-linux-gnu-ld"
LIBGCC=$(i686-linux-gnu-gcc -print-libgcc-file-name)

echo "=== corrOS build ==="

# Step 1: Compile shell.cro to C
echo "[1/4] Compiling shell.cro to C..."
rm -f /tmp/corros-codegen-*.bc /tmp/corros-vm-state.bin
CORROS_KEEP_C=1 "$CORROS" --compile src/shell.cro 2>/dev/null
CFILE=$(ls -t /tmp/corros-c-*.tmp 2>/dev/null | head -1)
if [ -z "$CFILE" ]; then
  echo "  ERROR: corros --compile failed"; exit 1
fi
cp "$CFILE" src/shell_generated.c
sed -i 's/int main(void)/void kernel_main(void)/' src/shell_generated.c
sed -i 's/return 0;//g' src/shell_generated.c
echo "  -> src/shell_generated.c"

# Step 2: Compile
echo "[2/4] Compiling to object files..."
CFLAGS="-ffreestanding -nostdlib -nostdinc -m32 -fno-pie -fno-stack-protector -O2 -Iinclude -w"
$CC $CFLAGS -c src/runtime.c -o build/runtime.o
$CC $CFLAGS -c src/shell_generated.c -o build/shell_generated.o
echo "  -> build/runtime.o build/shell_generated.o"

# Step 3: Link
echo "[3/4] Linking kernel..."
$LD -m elf_i386 -T linker.ld -o build/corros_kernel.elf \
  build/runtime.o build/shell_generated.o "$LIBGCC"

echo ""
echo "=== Build complete ==="
echo "Kernel ELF: build/corros_kernel.elf ($(wc -c < build/corros_kernel.elf) bytes)"
echo ""
echo "Run with QEMU:"
echo "  qemu-system-i386 -kernel build/corros_kernel.elf"
