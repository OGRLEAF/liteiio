#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"

#ifndef _BBIO_H

#define _BBIO_H

#define MAX_DEVICE 4

typedef int IO_FD;

typedef struct _bbio_context_base io_context;

enum IO_DEVICE_TYPE
{
    MAPPED_DEVICE = 0,
    BUFFER_DEVICE = 1,
};

typedef struct _bbio_device_mapped_channel
{
    io_write_call write;
    io_read_call read;
    uint32_t * start;
    uint32_t * end;
} io_mapped_channel;

typedef struct _bbio_device_stream_channel
{
    io_write_stream write_stream;
    io_read_stream read_stream;
    void * private;
} io_stream_channel;


typedef struct _bbio_device_base
{
    uint16_t id;
    uint16_t type;
    io_context *ctx;
    char *path;
    void *mmap_mirror;
} io_device;

typedef struct _bbio_stream_device
{
    io_device device;
    io_stream_channel ch;
    IO_FD fd;
    uint16_t buffer_n;
    // io_stream_buffer * buffer_pool;
    uint16_t current_buffer;
} io_stream_device;

typedef struct _bbio_mapped_device
{
    io_device device;
    io_mapped_channel ch;
    int fd;
} io_mapped_device;

typedef IO_FD (*io_open_call)(io_context *ctx, char *file, int flag);
typedef int (*io_close_call)(io_context *ctx, IO_FD fd);
typedef void *(*io_mmap_call)(io_context *ctx, void **addr, IO_FD fd, size_t __len);

typedef io_mapped_device *(*io_open_mapped_channel_call)(io_context *ctx, char *file_path, size_t size);
typedef io_stream_device *(*io_open_stream_channel_call)(io_context *ctx, char *file_path);

typedef int (*io_ioctl_call)(io_context *ctx, int __fd, unsigned long __request, void *payload);

typedef void (*io_write_call)(io_mapped_device *device, uint32_t addr, uint32_t value);
typedef uint32_t (*io_read_call)(io_mapped_device *device, uint32_t addr);

typedef uint32_t (*io_write_stream)(io_stream_device * device, void *data, uint32_t size);
typedef uint32_t (*io_read_stream)(io_stream_device * device, void *data, uint32_t size);

typedef uint16_t (*io_write_busrt_call)(io_context *ctx, void *addr, void *data, uint32_t size);
typedef uint16_t (*io_read_busrt_call)(io_context *ctx, void *addr, void *data, uint32_t size);

typedef void (*io_sync_buffer)(io_context *ctx, void *addr, void *data, int16_t size, uint8_t dir);

typedef struct _bbio_context_backend
{
    io_open_mapped_channel_call open_mapped;
    io_open_stream_channel_call open_stream;
    // io_open_call open;
    // io_close_call close;
    // io_mmap_call mmap;
    // io_ioctl_call ioctl;
    // io_write_call write;
    // io_read_call read;
    // io_write_busrt_call write_burst;
    // io_read_busrt_call read_burst;
    // io_sync_buffer sync_buffer;
} io_backend;

typedef struct _bbio_context_base
{
    io_device **devices;
    io_backend backend;
} io_context;

typedef struct _bbio_stream_buffer
{
    uint32_t id;
    uint32_t size;
    uint8_t busy;
    char *buffer;
} io_stream_buffer;

typedef struct _bbio_stream_buffer_task_
{
    struct channel_buffer *channel_buffer;
    int buffer_id;
} io_stream_task;

io_context *io_create_context();
io_context *io_create_net_context(char *host, uint16_t port);

io_device *io_add_mapped_device(io_context *ctx, char *path);
io_device *io_add_stream_device(io_context *ctx, char *path);

struct channel_buffer_context *io_buffer_allocate(io_stream_device *device, int buffer_count, int channel_fd);

uint32_t io_read_mapped_device(io_mapped_device *device, uint32_t addr);
void io_write_mapped_device(io_mapped_device *device, uint32_t addr, uint32_t value);

void io_write_stream_device(io_stream_device *device, uint32_t addr, uint32_t value);

uint32_t io_read_stream_device(io_stream_device *device, uint32_t addr);
io_stream_task io_stream_request_buffer(io_stream_device *device);
uint32_t io_stream_commit(io_stream_device *device, io_stream_task *task);
void io_stream_sync_buffer_ext(io_stream_device *device, uint32_t dest_addr, struct channel_buffer *src, uint32_t size);
#endif