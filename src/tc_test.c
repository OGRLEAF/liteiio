#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <argp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <linux/types.h>
#include <sys/mman.h>
#include "bbio_h.h"

#define TEST_SIZE 512

int main_(int argc, char **argv)
{
    printf("sizeof pointer %ld %ld %ld\n", sizeof(uint32_t *), sizeof(iq_buffer), sizeof(struct channel_buffer));

    printf("Create context\n");
    int i, ret;
    io_context *ctx = io_create_context();
    printf("Add devices\n");
    io_mapped_device *dev = (io_mapped_device *)io_add_mapped_device(ctx, "/dev/tc");

    // Read / Write test
    for (i = 0; i < 512; i++)
    {
        io_write_mapped_device(dev, i, i);
        ret = io_read_mapped_device(dev, i);
    }

    for (i = 0; i < 512; i++)
    {
        ret = io_read_mapped_device(dev, i);
    }

    io_stream_device *dma_dev = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_proxy_tx");
    io_stream_task task = io_stream_request_buffer(dma_dev);

    struct channel_buffer *buffer_test = (struct channel_buffer *)malloc(sizeof(struct channel_buffer));

    int test_size = 4096;
    for (i = 0; i < test_size; i++)
    {
        buffer_test->buffer[i].I0 = i;
        buffer_test->buffer[i].Q0 = i;
    }
    // task.channel_buffer->length = 1024;

    io_stream_sync_buffer_ext(dma_dev, task.buffer_id, buffer_test, test_size);

    io_stream_commit(dma_dev, &task);

    // task = io_stream_request_buffer(dma_dev);

    // for (i = 0; i < 1024; i++)
    // {
    //     task.channel_buffer->buffer[i].I0 = 1024 - i;
    //     task.channel_buffer->buffer[i].Q0 = i;
    // }
    // task.channel_buffer->length = 1024;
    // io_stream_commit(dma_dev, &task);

    // task = io_stream_request_buffer(dma_dev);

    // for (i = 0; i < 1024; i++)
    // {
    //     task.channel_buffer->buffer[i].I0 = 1024 - i;
    //     task.channel_buffer->buffer[i].Q0 = i;
    // }
    // task.channel_buffer->length = 1024;
    // io_stream_commit(dma_dev, &task);

    return 0;
}

int main(int argc, char **argv)
{
    printf("sizeof pointer=%ld enum=%ld\n", sizeof(uint32_t *), sizeof(PROXY_BUSY));
    printf("Create context\n");
    int i, ret;
    io_context *ctx = io_create_context(); //_net_context("192.168.2.5", 12345);
    printf("Add devices\n");
    // io_mapped_device *dev = (io_mapped_device *)io_add_mapped_device(ctx, "/dev/tc");

    // io_write_mapped_device(dev, 0, 1234);

    // int read_value = io_read_mapped_device(dev, 0);
    // printf("Read value=%d\n", read_value);

    io_stream_device *dma_dev = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_proxy_tx");
    io_stream_task task = io_stream_request_buffer(dma_dev);

    struct channel_buffer *buffer_test = (struct channel_buffer *)malloc(sizeof(struct channel_buffer));

    int test_size = MAX_SAMPLES;
    for (i = 0; i < test_size; i++)
    {
        buffer_test->buffer[i].I0 = i;
        buffer_test->buffer[i].Q0 = i;
    }
    // task.channel_buffer->length = 1024;
    printf("Sync buffer samples=%d\n", test_size);

    io_stream_sync_buffer_ext(dma_dev, task.buffer_id, buffer_test, test_size);
    io_stream_commit(dma_dev, &task);
    while (1)
    {
        // printf("Channel %d/%d\n", task.buffer_id, dma_dev->buffer_n);
        task = io_stream_request_buffer(dma_dev);
        io_stream_sync_buffer_ext(dma_dev, task.buffer_id, buffer_test, test_size);
        io_stream_commit(dma_dev, &task);
    }

    return 0;
}