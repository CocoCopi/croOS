# croOS App Extension Format

## File Extensions

| Extension | Description |
|-----------|------------|
| `.cro` | Corros source code |
| `.cropkg` | Compiled Corros package (binary) |
| `.croapp` | App metadata + compiled binary |
| `.exe` | Windows executable (compatibility layer) |
| `.deb` | Debian package (compatibility layer) |
| `.apk` | Android package (compatibility layer) |

## .cropkg Format

```
Header (16 bytes):
  Magic: "CORP" (4 bytes)
  Version: uint8 (1 byte)
  Entry offset: uint32 (4 bytes)
  Code size: uint32 (4 bytes)
  Data size: uint32 (4 bytes)

Code section:
  Compiled Corros bytecode or C object code

Data section:
  String literals, static data
```

## .croapp Format

```
Header (64 bytes):
  Magic: "CORA" (4 bytes)
  App name: char[32]
  Version: char[8]
  Author: char[16]
  Entry offset: uint32
  Code size: uint32
  Icon: 4x4 character art (16 bytes)

Compiled binary follows
```

## Building an App

```bash
# 1. Write your app in .cro
echo 'craft main(){speak("Hello from croOS!")}' > hello.cro

# 2. Compile with --compile
corros --compile hello.cro

# 3. Package as .cropkg
corros --package hello.cro -o hello.cropkg

# 4. Install to croOS
corros --install hello.cropkg
```

## App API (via lib/app.cro)

Apps use these builtins for I/O:
- `poke8(addr, val)` — write to VGA memory
- `peek8(addr)` — read from keyboard port
- `speak(str)` — print to console
- `str(num)` — convert number to string
- `tick()` — timer tick
- `mem_alloc(n)` / `mem_free(p)` — memory management
- `$slice(s, a, b)` — substring
- `$len(s)` — string length

## Compatibility Layers

### .exe (Windows)
Translates Win32 API calls to croOS syscalls:
- `CreateFile` → VFS operations
- `ReadFile` / `WriteFile` → VFS read/write
- `MessageBox` → VGA text output
- `GetAsyncKeyState` → keyboard port read

### .deb (Debian)
Extracts and runs Debian packages:
- Parses `control` file for metadata
- Extracts `data.tar.gz` to VFS
- Runs `postinst` script via shell

### .apk (Android)
Extracts Android packages:
- Parses `AndroidManifest.xml`
- Extracts `classes.dex` (Dalvik bytecode)
- Translates to Corros bytecode at install time
