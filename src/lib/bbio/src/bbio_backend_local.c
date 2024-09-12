#include "bbio_h.h"
#include "bbio_private_h.h"
#include "buffer.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

typedef struct _local_buffer_description
{
    struct channel_buffer_context *buffer_ctx;
    uint32_t buffer_count;
    uint32_t current_buffer_id;
} local_buffer_d;

IO_FD io_open_local(io_context *ctx, char *file, int flag)
{
    return open(file, flag);
}

int io_close_local(io_context *ctx, IO_FD fd)
{
    return close(fd);
}

void *io_mmap_local(io_context *ctx, void **addr, IO_FD fd, size_t __len)
{
    *addr = mmap(NULL, __len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return *addr;
}

int io_ioctl_local(io_context *ctx, int __fd, unsigned long __request, void *payload)
{
    return ioctl(__fd, __request, payload);
}

void io_write_local(io_mapped_device *device, uint32_t addr, uint32_t value)
{
    if ((((uint64_t)addr)) > (device->ch.end - device->ch.start))
    {
        return;
    }
    uint32_t *p = (uint32_t *)(device->ch.start + addr);
    *p = value;
}

uint32_t io_read_local(io_mapped_device *device, uint32_t addr)
{
    if ((((uint64_t)addr)) > (device->ch.end - device->ch.start))
    {
        return -1;
    }
    uint32_t *p = (uint32_t *)(device->ch.start + addr);
    return *p;
}

uint16_t io_write_busrt_local(io_context *ctx, void *addr, void *data, uint32_t size)
{
    memcpy((void *)addr, (void *)data, size);
    return size;
}

uint16_t io_read_busrt_local(io_context *ctx, void *addr, void *data, uint32_t size)
{
    memcpy((void *)data, (void *)addr, size);
    return size;
}

static local_buffer_d *io_local_buffer_init(int fd)
{
    struct channel_buffer_context *buffer_ctx = buffer_allocate(BUFFER_COUNT, fd);
    if (!buffer_ctx)
    {
        return NULL;
    }

    local_buffer_d *local_buffer = (local_buffer_d *)malloc(sizeof(local_buffer_d));
    local_buffer->buffer_count = BUFFER_COUNT;
    local_buffer->buffer_ctx = buffer_ctx;
    return local_buffer;
}

static uint32_t io_write_stream_local(io_stream_device *device, void *data, uint32_t size)
{
    local_buffer_d *local_buffer = (local_buffer_d *)device->ch.private;
    struct channel_buffer *current_buffer = local_buffer->buffer_ctx->channel_buffer + local_buffer->current_buffer_id;
    printf("Buffer %d status:%d -> ", local_buffer->current_buffer_id, current_buffer->status);
    if (current_buffer->status == PROXY_QUEUED)
    {
        // wait for the last buffer finished, it means we are faster than dma
        ioctl(device->fd, FINISH_XFER, &local_buffer->current_buffer_id);
    }

    printf("%d\r", current_buffer->status);

    current_buffer->length = size;
    // zero copy support
    if (((void *)current_buffer) == data)
    {
    }
    else
        memcpy(current_buffer->buffer, data, size);

    // start next transmition
    ioctl(device->fd, START_XFER, &local_buffer->current_buffer_id);

    // rolling buffer
    local_buffer->current_buffer_id = (local_buffer->current_buffer_id + 1) % local_buffer->buffer_count;
}

struct channel_buffer *io_stream_zc_buffer_local(io_stream_device *device, uint32_t flag)
{
    // get next buffer
    local_buffer_d *local_buffer = (local_buffer_d *)device->ch.private;
    struct channel_buffer *current_buffer = local_buffer->buffer_ctx->channel_buffer + local_buffer->current_buffer_id;
    // printf("Buffer %d status:%d -> ", local_buffer->current_buffer_id, current_buffer->status);
    // if (current_buffer->status == PROXY_QUEUED)
    //     // wait for the last buffer finished, it means we are faster than dma
    //     ioctl(device->fd, FINISH_XFER, &local_buffer->current_buffer_id);
    // // may 3s timeout
    // // check buffer status
    // if(current_buffer->status == PROXY_TIMEOUT) {
    //     // try clear error
    // }
    // printf("%d\n", current_buffer->status);

    return current_buffer;
}

io_mapped_device *io_open_mapped_local(io_context *ctx, char *file_path, size_t size)
{
    io_mapped_device *device;
    int fd = open(file_path, O_RDWR);

    if (fd < 1)
    {
        printf("Device %s open error %d\n", file_path, fd);
        free(device);
        return NULL;
    }
    device = (io_mapped_device *)malloc(sizeof(io_mapped_device));
    device->ch.start = mmap(NULL, size * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (device->ch.start == NULL)
    {
        free(device);
        close(fd);
        return NULL;
    }
    device->fd = fd;
    device->ch.end = device->ch.start + size;
    device->ch.read = io_read_local;
    device->ch.write = io_write_local;
    return device;
}

io_stream_device *io_open_stream_local(io_context *ctx, char *file_path)
{
    io_stream_device *device;
    int fd;
    fd = open(file_path, O_RDWR);

    if (fd < 1)
    {
        printf("Device %s open error %d\n", file_path, fd);
        return NULL;
    }

    device = (io_stream_device *)malloc(sizeof(io_stream_device));

    device->fd = fd;
    device->ch.private = io_local_buffer_init(fd);
    device->ch.write_stream = io_write_stream_local;
    device->ch.alloc_buffer = io_stream_zc_buffer_local;
    if (device->ch.private == NULL)
    {
        free(device);
        close(fd);
        return NULL;
    }
    return device;
}