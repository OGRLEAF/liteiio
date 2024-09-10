#include "buffer.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "bbio_h.h"
#include <time.h>


#define _dma_buffer_attr_off(buffer, id, attr) (((void *)&buffer[id].attr) - (void *)buffer)
#define _dma_buffer_attr(device, id, attr) (io_read_stream_device(device, _dma_buffer_attr_off((device->buffer->channel_buffer), id, attr)))
#define _dma_buffer_attr_set(device, id, attr, value) (io_write_stream_device(device, _dma_buffer_attr_off((device->buffer->channel_buffer), id, attr), value))

#define _dma_status(device, id) (_dma_buffer_attr(device, id, status))
#define _dma_length(device, id) (_dma_buffer_attr(device, id, length))
#define _dma_length_set(device, id, value) _dma_buffer_attr_set(device, id, length, value)

struct channel_buffer_context *io_buffer_allocate(io_stream_device *device, int buffer_count, int channel_fd)
{
    io_context *ctx = device->device.ctx;
    struct channel_buffer * channel_buffer = (struct channel_buffer *)mmap(NULL, sizeof(struct channel_buffer) * buffer_count,
                                                            PROT_READ | PROT_WRITE, MAP_SHARED, channel_fd, 0);
    // device->device.mmap_mirror = ctx->backend.mmap(ctx, (void **)&channel_buffer, channel_fd,
    //                                                sizeof(struct channel_buffer) * buffer_count);

    printf("Buffer allocated %p > %p\n", device->device.mmap_mirror, channel_buffer);
    if (channel_buffer == MAP_FAILED)
    {
        return NULL;
    }
    struct channel_buffer_context *context = (struct channel_buffer_context *)malloc(sizeof(struct channel_buffer_context));
    context->channel_buffer = channel_buffer;

    return context;
}

io_stream_task io_stream_request_buffer(io_stream_device *device)
{
    int next_buffer_id = (device->current_buffer);
    io_stream_task task = {
        .channel_buffer = (device->buffer->channel_buffer + next_buffer_id),
        .buffer_id = next_buffer_id};

    if (_dma_status(device, next_buffer_id) == PROXY_QUEUED)
    {
        // printf("Buffer %d has been queued, wait for finishing...", next_buffer_id);
        device->device.ctx->backend.ioctl(device->device.ctx, device->fd, FINISH_XFER, &next_buffer_id);
        // printf("status -> %d\n", _dma_status(device, next_buffer_id));
    }
    return task;
}

uint32_t io_stream_commit(io_stream_device *device, io_stream_task *task)
{

    // device->device.ctx->backend.ioctl(device->device.ctx, device->fd, FINISH_XFER, &task->buffer_id);
    // printf("Status=%d length=%d\n", _dma_status(device, task->buffer_id), _dma_length(device, task->buffer_id)
    //     // task->channel_buffer->length
    // );
    // device->device.ctx->backend.ioctl(device->device.ctx, device->fd, FINISH_XFER, &task->buffer_id);
    // if (_dma_status(device, task->buffer_id) == PROXY_BUSY)
    // {
    //     device->device.ctx->backend.ioctl(device->device.ctx, device->fd, FINISH_XFER, &task->buffer_id);
    // if (_dma_status(device, task->buffer_id) != PROXY_NO_ERROR)
    // {
    //     printf("Proxy tx transfer error at buffer=%d err=%d\n", task->buffer_id, task->channel_buffer->status);
    //     return -1;
    // }
    // }

    device->device.ctx->backend.ioctl(device->device.ctx, device->fd, START_XFER, &task->buffer_id);
    device->current_buffer =  (device->current_buffer + 1 ) % device->buffer_n;

    return 0;
}

void io_sync_stream_buffer(io_stream_device *device, uint32_t addr, int16_t size)
{
    // return device->mapped_regs[addr];
    // if (device->device.ctx->backend.sync_buffer != NULL)
    // {
    //     return device->device.ctx->backend.sync_buffer(device->device.ctx, (void *)device->buffer->channel_buffer + addr,
    //                                                    (void *) device->device.mmap_mirror + addr,
    //                                                    size, 0);
    // }
}

void io_stream_sync_buffer_ext(io_stream_device *device, uint32_t buffer_id, struct channel_buffer *src, uint32_t size)
{
    // return device->mapped_regs[addr];
    clock_t begin, end;
    double time_spent;
    src->length = size * sizeof(iq_buffer);
    device->device.ctx->backend.write_burst(
        device->device.ctx,
        (void *)(device->buffer->channel_buffer + buffer_id),
        (void *)src, src->length);

    // printf("Transmit %d speed : %.2lfkB/s\n", src->length, src->length / time_spent * 1e3 / 1024 );
    _dma_length_set(device, buffer_id, src->length);
}
