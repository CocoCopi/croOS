# corrOS

A bare-metal operating system kernel written **entirely in Corros** (`.cro` files), compiled to freestanding C via `corros --compile`, then linked as a 32-bit i386 ELF.

## Architecture

```
shell.cro          ← the kernel, written in Corros
    ↓ corros --compile
shell_generated.c  ← C code emitted by the Corros C backend
    ↓ i686-linux-gnu-gcc -ffreestanding -nostdlib
shell_generated.o  ← freestanding object file
    ↓ i686-linux-gnu-ld -T linker.ld
corros_kernel.elf  ← bootable kernel image
```

**Zero Rust, zero Go, zero hand-written C in the kernel.** The entire kernel logic — VGA text output, keyboard polling, memory tests, boot banner — is Corros source code that compiles to C through the `--compile` pipeline.

### What the kernel does
- **Boots** via the linker entry point (`_start` → `kernel_main`)
- **Clears the screen** using `poke8()` to VGA text memory at `0xB8000`
- **Writes a boot banner** via `speak()` (Corros builtin → `printf` → VGA)
- **Memory tests**: `mem_alloc`, `poke`/`poke8`/`peek`/`peek8`/`poke16`/`peek16`
- **Reads keyboard** via `peek8(0x60)` (PS/2 scancode port)
- **Timer** via `tick()` (TSC-based)

### Builtins used (Corros language features)
| Builtin | Purpose in corrOS |
|---|---|
| `speak()` | Write text to console (VGA) |
| `poke(addr, val)` | Write 32-bit value to memory |
| `poke8(addr, val)` | Write byte to memory (VGA, keyboard) |
| `poke16(addr, val)` | Write 16-bit value |
| `peek(addr)` | Read 32-bit value from memory |
| `peek8(addr)` | Read byte from memory |
| `peek16(addr)` | Read 16-bit value |
| `mem_alloc(n)` | Allocate n bytes (bump allocator) |
| `mem_free(p)` | Free allocated memory |
| `str(num)` | Convert number to string |
| `tick()` | Get timer tick |

## Prerequisites

- `corros` (the Corros compiler) — in `$PATH`
- `i686-linux-gnu-gcc` — for cross-compilation to i386
- `i686-linux-gnu-ld` — the cross-linker

## Build

```bash
chmod +x build.sh
./build.sh
```

## Run

```bash
qemu-system-i386 -kernel build/corros_kernel.elf
```

## Adding new commands

Edit `src/shell.cro`, add your logic using only Corros builtins (no external C functions), then rebuild. The C backend constraint: avoid control-flow joins with live variables (no `when`/`else` blocks that assign to the same variable and use it after).

## License

MIT
