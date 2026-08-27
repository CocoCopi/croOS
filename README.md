# croOS

**A complete operating system kernel written from scratch.**

croOS is a full 32-bit x86 operating system with memory management, process scheduling, a virtual filesystem, TCP/IP networking, 42 system calls, and 16 built-in applications — all compiled from the Corros programming language (`.cro` files).

**GitHub:** [github.com/CocoCopi/croOS](https://github.com/CocoCopi/croOS)

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       User Space                            │
│  ┌──────┐ ┌────────┐ ┌──────┐ ┌────────┐ ┌──────┐         │
│  │ Calc │ │ Snake  │ │Notepad│ │Browser │ │ ...  │  16 Apps │
│  └──┬───┘ └───┬────┘ └──┬───┘ └───┬────┘ └──┬───┘         │
│     └─────────┴─────────┴─────────┴──────────┘              │
│                        INT 0x80                              │
├─────────────────────────────────────────────────────────────┤
│                       Kernel Space                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Process Manager │ Scheduler │ 42 Syscalls           │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  VFS Layer  │  Ramdisk FS  │  FAT16 (TODO)          │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  TCP/IP Stack │ ARP │ ICMP │ TCP │ UDP               │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  PMM (bitmap) │ VMM (paging) │ Heap allocator        │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  GDT │ IDT │ ISR/IRQ │ PIC │ PIT Timer │ Keyboard   │   │
│  └──────────────────────────────────────────────────────┘   │
│                      0xB8000 VGA                            │
└─────────────────────────────────────────────────────────────┘
```

## Kernel Features

### Memory Management
- **Physical Memory Manager (PMM):** Bitmap-based page frame allocator, supports up to 128MB
- **Virtual Memory Manager (VMM):** x86 two-level page tables with identity mapping and user-space support
- **Kernel Heap (kmalloc):** First-fit allocator with block splitting and coalescing, 32MB heap

### Process Management
- **Round-robin scheduler** with per-process priority
- **64 concurrent processes** with independent page directories
- **Context switching** via saved register state (EIP, ESP, segment registers, EFLAGS)
- **fork/exec** support for process creation

### Interrupt Handling
- **256 IDT entries** — ISRs 0-31 (CPU exceptions) + IRQs 0-15 (hardware)
- **ISR stubs** in assembly, dispatching to C handlers
- **PIC remapping** — IRQ 0-7 → INT 32-47

### Drivers
- **VGA text-mode:** 80x25, 16 colors, cursor control, scrolling, rectangle drawing
- **PS/2 Keyboard:** Scancode set 1, shift/ctrl/alt modifiers, ring buffer
- **PIT Timer:** 100 Hz tick, millisecond sleep
- **Serial (COM1):** Debug output for QEMU (`-serial stdio`)

### Filesystem
- **VFS layer:** Mount-based routing, file descriptors, stat/mkdir/rmdir/rename
- **Ramdisk:** In-memory filesystem with create/read/write/delete, 64MB max

### Networking
- **TCP/IP stack:** ARP resolution, ICMP echo (ping), TCP connections, UDP datagrams
- **Ethernet frame I/O:** Send/receive via NIC driver callback
- **ARP cache:** 16-entry LRU, automatic resolution

### System Calls
- 42 system calls via `INT 0x80`
- File I/O, process management, memory allocation, VGA output, keyboard input, networking, power management

## Built-in Applications (all in Corros `.cro`)

| Key | App       | Description                          |
|-----|-----------|--------------------------------------|
| `c` | Calculator | Arithmetic: +, -, *, /               |
| `l` | Calendar  | Monthly calendar view                |
| `t` | Clock     | Live timer display                   |
| `n` | Notepad   | Free-form text input                 |
| `s` | Snake     | Classic snake game                   |
| `r` | Tetris    | Falling block puzzle                 |
| `g` | 2048      | Slide tiles to 2048                  |
| `m` | Minesweeper | 8x8 mine detection                |
| `p` | Pong      | AI opponent paddle game              |
| `b` | Browser   | Text-mode web browser                |
| `i` | Image Viewer | VGA character graphics            |
| `k` | Camera    | VGA framebuffer capture              |
| `e` | File Explorer | VFS directory navigation         |
| `x` | Text Editor | Freeform text editing             |
| `d` | Doc Viewer | Multi-format document viewer        |
| `a` | App Manager | Installed apps list                |

## Build

### Prerequisites
```bash
apt-get install -y gcc-i686-linux-gnu nasm qemu-system-x86
```

### Build the kernel
```bash
cd croOS
make
```

### Run in QEMU
```bash
make run
```

### Build for real hardware
```bash
make
# Write build/croOS.elf to disk partition
dd if=build/croOS.elf of=/dev/sdX bs=512
```

## File Structure

```
croOS/
├── Makefile                    # Build system
├── linker.ld                   # Linker script
├── src/
│   ├── kernel/
│   │   ├── asm/
│   │   │   ├── boot.S          # Multiboot entry + stack
│   │   │   ├── gdt_flush.S     # GDT segment reload
│   │   │   └── isr.S           # 256 ISR/IRQ stubs
│   │   ├── gdt.c/h             # Global Descriptor Table
│   │   ├── idt.c/h             # Interrupt Descriptor Table
│   │   ├── process.c/h         # Process manager + scheduler
│   │   ├── kmain.c             # Kernel entry point + shell
│   │   └── types.h             # Fundamental types + port I/O
│   ├── mm/
│   │   ├── pmm.c/h             # Physical memory (bitmap pages)
│   │   ├── vmm.c/h             # Virtual memory (x86 paging)
│   │   └── kmalloc.c/h         # Kernel heap (first-fit free list)
│   ├── drivers/
│   │   ├── vga.c/h             # VGA text-mode (80x25)
│   │   ├── keyboard.c/h        # PS/2 keyboard (scancode set 1)
│   │   ├── timer.c             # PIT timer (100 Hz) + TSC
│   │   └── serial.c/h          # COM1 serial (QEMU debug)
│   ├── fs/
│   │   ├── vfs.c/h             # Virtual filesystem layer
│   │   └── ramdisk.c/h         # In-memory filesystem
│   ├── net/
│   │   ├── net.c/h             # TCP/IP stack (ARP/ICMP/TCP/UDP)
│   ├── sys/
│   │   └── syscall.c/h         # 42 system calls (INT 0x80)
│   └── libc/
│       └── string.c            # C standard library (string/math)
├── apps/
│   ├── calc.cro                # Calculator
│   └── ...
├── lib/
│   └── app.cro                 # App framework
├── doc/
│   └── APP_FORMAT.md           # App extension format docs
└── README.md
```

## App Extension Format

- `.cro` — Corros source code
- `.cropkg` — Compiled Corros package (binary)
- `.croapp` — App metadata + compiled binary
- `.exe` — Windows compatibility layer
- `.deb` — Debian package compatibility
- `.apk` — Android package compatibility

## Language: Corros

croOS is built with [Corros](https://github.com/CocoCopi/corros), a self-hosting, bytecode-compiled programming language. The kernel's high-level logic is written in `.cro` files and compiled to C via `corros --compile`, then cross-compiled to i386 with GCC.

## License

MIT
