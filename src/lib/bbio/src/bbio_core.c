#include "bbio_h.h"
#include "bbio_private_h.h"
#include "bbio_backend.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

io_context *io_create_context()

{
    io_context *ctx = (io_context *)malloc(sizeof(io_context));
    ctx->devices = (io_device **)malloc(1 * sizeof(io_device *));
    ctx->devices[0] = NULL;
    ctx->backend.open_mapped = io_open_mapped_local;
    ctx->backend.open_stream = io_open_stream_local;
    // ctx->backend.open = io_open_local;
    // ctx->backend.close = io_close_local;
    // ctx->backend.mmap = io_mmap_local;
    // ctx->backend.ioctl = io_ioctl_local;
    // ctx->backend.read = io_read_local;
    // ctx->backend.write = io_write_local;
    // ctx->backend.write_burst = io_write_busrt_local;
    // ctx->backend.read_burst = io_read_busrt_local;
    // ctx->backend.sync_buffer = NULL;
    return ctx;
};


io_device *io_add_mapped_device(io_context *ctx, char *path)
{
    io_mapped_device *device = ctx->backend.open_mapped(ctx, path, 512);
    if (io_append_device(ctx, (io_device *)device) <= 0)
    {
        free(device);
        return NULL;
    }

    return (io_device *)device;
}

io_device *io_add_stream_device(io_context *ctx, char *path)
{
    io_stream_device *device = ctx->backend.open_stream(ctx, path);
    if (io_append_device(ctx, (io_device *)device) <= 0)
    {
        free(device);
        return NULL;
    }

    return (io_device *)device;
}

void io_write_mapped_device(io_mapped_device *device, uint32_t addr, uint32_t value)
{
    return device->ch.write(device, addr, value);
}

uint32_t io_read_mapped_device(io_mapped_device *device, uint32_t addr)
{
    // return device->mapped_regs[addr];
    return device->ch.read(device, addr);
}

struct channel_buffer *io_stream_get_buffer(io_stream_device *device)
{
    return device->ch.alloc_buffer(device, 0);
}

void io_write_stream_device(io_stream_device *device, void *data, uint32_t size)
{
    device->ch.write_stream(device, data, size);
}
