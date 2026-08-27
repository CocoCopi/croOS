/* croOS stdio.c - Standard I/O library implementation
 * printf, fprintf, sprintf, fopen, fread, fwrite, fclose, fgets, etc. */

#include "kernel/types.h"
#include "string.h"
#include "drivers/vga.h"
#include "fs/vfs.h"
#include "mm/kmalloc.h"

/* printf family */
int printf(const char *fmt, ...) {
    char buf[2048];
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    __builtin_va_end(args);
    vga_puts(buf);
    return len;
}

int fprintf(int fd, const char *fmt, ...) {
    char buf[2048];
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    __builtin_va_end(args);
    if (fd == 1 || fd == 2) {
        /* stdout/stderr → VGA */
        vga_puts(buf);
    } else {
        vfs_write(fd, buf, len);
    }
    return len;
}

int sprintf(char *buf, const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int len = vsnprintf(buf, 4096, fmt, args);
    __builtin_va_end(args);
    return len;
}

int vsprintf(char *buf, const char *fmt, __builtin_va_list args) {
    return vsnprintf(buf, 4096, fmt, args);
}

int puts(const char *s) {
    vga_puts(s);
    vga_putchar('\n');
    return strlen(s) + 1;
}

int putchar(int c) {
    vga_putchar((char)c);
    return c;
}

/* File operations */
typedef struct {
    int fd;
    int mode;
    char path[256];
    int error;
    int eof;
    int pos;
} FILE;

static FILE std_files[3] = {
    { .fd = -1, .mode = 0, .path = "stdin" },
    { .fd = -1, .mode = 1, .path = "stdout" },
    { .fd = -1, .mode = 1, .path = "stderr" },
};

FILE *stdin_file  = &std_files[0];
FILE *stdout_file = &std_files[1];
FILE *stderr_file = &std_files[2];

FILE *fopen(const char *path, const char *mode) {
    uint8_t flags = 0;
    if (mode[0] == 'r') flags = VFS_MODE_READ;
    else if (mode[0] == 'w') flags = VFS_MODE_WRITE | VFS_MODE_CREATE;
    else if (mode[0] == 'a') flags = VFS_MODE_APPEND | VFS_MODE_CREATE;

    int fd = vfs_open(path, flags);
    if (fd < 0) return NULL;

    FILE *f = (FILE*)kmalloc(sizeof(FILE));
    if (!f) { vfs_close(fd); return NULL; }
    f->fd = fd;
    f->mode = mode[0] == 'r' ? 0 : 1;
    f->error = 0;
    f->eof = 0;
    f->pos = 0;
    strncpy(f->path, path, 255);
    return f;
}

int fclose(FILE *stream) {
    if (!stream) return -1;
    vfs_close(stream->fd);
    kfree(stream);
    return 0;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
    if (!stream) return 0;
    uint32_t total = size * count;
    int n = vfs_read(stream->fd, ptr, total);
    if (n <= 0) stream->eof = 1;
    return n > 0 ? (size_t)n : 0;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
    if (!stream) return 0;
    uint32_t total = size * count;
    int n = vfs_write(stream->fd, ptr, total);
    return n > 0 ? (size_t)n : 0;
}

int fseek(FILE *stream, long offset, int whence) {
    if (!stream) return -1;
    return vfs_seek(stream->fd, (int32_t)offset, whence);
}

long ftell(FILE *stream) {
    if (!stream) return -1;
    return stream->pos;
}

char *fgets(char *s, int size, FILE *stream) {
    if (!stream || size <= 0) return NULL;
    int pos = 0;
    while (pos < size - 1) {
        char c;
        int n = vfs_read(stream->fd, &c, 1);
        if (n <= 0) { stream->eof = 1; break; }
        s[pos++] = c;
        if (c == '\n') break;
    }
    s[pos] = '\0';
    return pos > 0 ? s : NULL;
}

int fgetc(FILE *stream) {
    char c;
    int n = fread(&c, 1, 1, stream);
    return n == 1 ? (int)(unsigned char)c : -1;
}

int feof(FILE *stream) {
    return stream ? stream->eof : 1;
}

int ferror(FILE *stream) {
    return stream ? stream->error : 1;
}

void perror(const char *msg) {
    vga_puts(msg);
    vga_puts(": error\n");
}

/* Temporary files */
FILE *tmpfile(void) {
    int fd = vfs_open("/tmp/tmpfile", VFS_MODE_CREATE | VFS_MODE_READ | VFS_MODE_WRITE);
    if (fd < 0) return NULL;
    FILE *f = (FILE*)kmalloc(sizeof(FILE));
    f->fd = fd;
    f->mode = 1;
    f->error = 0;
    f->eof = 0;
    return f;
}
