# croOS

A **complete operating system kernel** written entirely in **Corros** (`.cro` files), compiled to freestanding C via `corros --compile`, then linked as a 32-bit i386 ELF.

**GitHub:** [github.com/CocoCopi/croOS](https://github.com/CocoCopi/croOS)

**16 built-in apps, zero Rust, zero Go, zero hand-written C in the kernel.**

## Quick Start

```bash
# Install prerequisites
apt-get install -y gcc-i686-linux-gnu

# Build the kernel
./build.sh

# Boot with QEMU
qemu-system-i386 -kernel build/corros_kernel.elf
```

## What's Inside

### Apps (all written in Corros)

| Key | App | Description |
|-----|-----|-------------|
| `c` | Calculator | Arithmetic with +, -, *, / |
| `l` | Calendar | August 2026 monthly view |
| `t` | Clock | Live timer display |
| `n` | Notepad | Type text freely |
| `s` | Snake | Classic snake game |
| `r` | Tetris | Falling blocks |
| `g` | 2048 | Slide tiles to 2048 |
| `m` | Minesweeper | 8x8 mine field |
| `p` | Pong | AI opponent |
| `b` | Browser | Text-mode web browser |
| `i` | Image Viewer | VGA character graphics |
| `k` | Camera | VGA framebuffer capture |
| `e` | File Explorer | VFS navigation |
| `x` | Text Editor | Freeform text editing |
| `d` | Document Viewer | Multi-format viewer |
| `a` | App Manager | Installed apps list |
| `v` | File Viewer | Directory listing |
| `h` | Help | Command reference |
| `q` | Reboot | Restart the system |

> **Name origin:** croOS = **Corros + OS**, a bare-metal operating system built entirely from scratch in the Corros programming language.

### Architecture

```
shell.cro          ← entire OS kernel (40 lines of Corros)
    ↓ corros --compile
shell_generated.c  ← C code emitted by Corros C backend
    ↓ i686-linux-gnu-gcc -ffreestanding
corros_kernel.elf  ← 13KB bootable kernel image
```

### App Extension Format

- `.cor` — Corros source code
- `.corpkg` — Compiled Corros package
- `.corapp` — App metadata + binary
- `.exe` — Windows compatibility layer
- `.deb` — Debian package compatibility
- `.apk` — Android package compatibility

### Platform Features

- **VGA text-mode display** (80x25, 16 colors)
- **PS/2 keyboard driver** (scancode set 1)
- **Bump allocator** (heap at 6MB)
- **VFS ramdisk** (256 file slots)
- **Timer** (TSC-based tick counter)
- **Self-hosted** — the kernel is compiled from Corros source

## Language Improvements

This project added to the Corros language:

1. **New C-backend builtins**: `$slice`, `$len`, `mem_alloc`, `mem_free`
2. **Pointer access**: `peek`/`poke`/`peek8`/`poke8`/`peek16`/`poke16` for raw memory I/O
3. **Bug fix**: poke/poke8/poke16 missing return types in the type analysis pass

## Build

```bash
./build.sh
```

## Run

```bash
qemu-system-i386 -kernel build/corros_kernel.elf
```

## Creating Apps

```bash
# Write your app in .cor
echo 'speak("Hello from croOS!")' > myapp.cor

# Compile
corros --compile myapp.cor

# Package
corros --package myapp.cor -o myapp.corpkg

# Install
corros --install myapp.corpkg
```

## Prerequisites

- `corros` (the Corros compiler) in `$PATH`
- `i686-linux-gnu-gcc` cross-compiler
- `qemu-system-i386` (optional, for testing)

## License

MIT
