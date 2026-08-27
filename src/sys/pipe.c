/* croOS pipe.c - Inter-process communication via pipes
 * Kernel ring buffer pipes used for process communication and shell piping. */

#include "kernel/types.h"
#include "drivers/vga.h"
#include "string.h"

#define PIPE_BUFFER_SIZE 4096
#define MAX_PIPES 64

typedef struct {
    uint8_t  buffer[PIPE_BUFFER_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
    int      readers;
    int      writers;
    uint8_t  in_use;
} pipe_t;

static pipe_t pipes[MAX_PIPES];

void pipe_init(void) {
    memset(pipes, 0, sizeof(pipes));
}

int pipe_create(void) {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!pipes[i].in_use) {
            memset(&pipes[i], 0, sizeof(pipe_t));
            pipes[i].in_use = 1;
            pipes[i].readers = 1;
            pipes[i].writers = 1;
            return i;
        }
    }
    return -1;
}

int pipe_read(int pipe_id, void *buf, uint32_t size) {
    if (pipe_id < 0 || pipe_id >= MAX_PIPES || !pipes[pipe_id].in_use) return -1;

    pipe_t *p = &pipes[pipe_id];
    uint8_t *dst = (uint8_t*)buf;
    uint32_t bytes_read = 0;

    while (bytes_read < size) {
        if (p->count == 0) {
            if (p->writers == 0) return bytes_read > 0 ? (int)bytes_read : 0;
            asm volatile("hlt");
            continue;
        }
        dst[bytes_read] = p->buffer[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUFFER_SIZE;
        p->count--;
        bytes_read++;
    }
    return (int)bytes_read;
}

int pipe_write(int pipe_id, const void *buf, uint32_t size) {
    if (pipe_id < 0 || pipe_id >= MAX_PIPES || !pipes[pipe_id].in_use) return -1;

    pipe_t *p = &pipes[pipe_id];
    const uint8_t *src = (const uint8_t*)buf;
    uint32_t bytes_written = 0;

    while (bytes_written < size) {
        if (p->count >= PIPE_BUFFER_SIZE) {
            if (p->readers == 0) return -1;
            asm volatile("hlt");
            continue;
        }
        p->buffer[p->write_pos] = src[bytes_written];
        p->write_pos = (p->write_pos + 1) % PIPE_BUFFER_SIZE;
        p->count++;
        bytes_written++;
    }
    return (int)bytes_written;
}

void pipe_close(int pipe_id) {
    if (pipe_id < 0 || pipe_id >= MAX_PIPES) return;
    if (pipes[pipe_id].in_use) {
        pipes[pipe_id].writers--;
        if (pipes[pipe_id].writers <= 0 && pipes[pipe_id].readers <= 0)
            pipes[pipe_id].in_use = 0;
    }
}

uint32_t pipe_data_available(int pipe_id) {
    if (pipe_id < 0 || pipe_id >= MAX_PIPES) return 0;
    return pipes[pipe_id].count;
}
