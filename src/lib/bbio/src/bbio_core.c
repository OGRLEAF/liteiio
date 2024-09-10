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
    io_stream_device *device;
    int fd;
    fd = ctx->backend.open(ctx, path, O_RDWR);

    if (fd < 1)
    {
        printf("Device %s open error %d\n", path, fd);
        return NULL;
    }

    device = (io_stream_device *)malloc(sizeof(io_stream_device));
    if (io_append_device(ctx, (io_device *)device) <= 0)
    {
        free(device);
        ctx->backend.close(ctx, fd);
        return NULL;
    }
    device->fd = fd;

    device->buffer_n = BUFFER_COUNT;
    device->current_buffer = 0;
    device->buffer = io_buffer_allocate(device, BUFFER_COUNT, fd);

    if (device->buffer == NULL)
    {
        free(device);
        ctx->backend.close(ctx, fd);
        return NULL;
    }
    return (io_device *)device;
}

void io_write_mapped_device(io_mapped_device *device, uint32_t addr, uint32_t value)
{
    return device->ch.write(device->device.ctx, addr, value);
}

uint32_t io_read_mapped_device(io_mapped_device *device, uint32_t addr)
{
    // return device->mapped_regs[addr];
    return device->ch.read(device, addr);
}

// void io_write_stream_device(io_stream_device *device, uint32_t addr, uint32_t value)
// {
//     void *prep = ((void *)device->buffer->channel_buffer) + addr;
//     if (prep == NULL)
//     {
//         perror("NULL pointer\n");
//         exit(-1);
//     }
//     return device->device.ctx->backend.write(device->device.ctx, (uint32_t *)prep, value);
// }

// uint32_t io_read_stream_device(io_stream_device *device, uint32_t addr)
// {
//     // return device->mapped_regs[addr];
//     void *prep = ((void *)device->buffer->channel_buffer) + addr;
//     if (prep == NULL)
//     {
//         perror("NULL pointer\n");
//         exit(-1);
//     }
//     return device->device.ctx->backend.read(device->device.ctx, (uint32_t *)prep);
// }
