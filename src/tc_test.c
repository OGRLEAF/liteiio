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

void io_finish_stream_local(io_stream_device *device);

int main_(int argc, char **argv)
{
    printf("sizeof pointer=%ld enum=%ld\n", sizeof(uint32_t *), sizeof(PROXY_BUSY));
    printf("Create context\n");
    int i, ret;
    io_context *ctx = io_create_context(); //_net_context("192.168.2.5", 12345);
    printf("Add devices\n");

    io_stream_device *dma_tx = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_proxy_tx");

    int test_times = 1, j = 0;

    int test_size = MAX_SAMPLES;
    for (int j = 0; j < test_times; j++)
    {
        struct channel_buffer *buffer_test = io_stream_get_buffer(dma_tx);

        for (i = 0; i < test_size; i++)
        {
            buffer_test->buffer[i].I0 = i + j * test_size;
            buffer_test->buffer[i].Q0 = i + j * test_size;
        }
        io_write_stream_device(dma_tx, buffer_test, test_size * sizeof(iq_buffer));
    }

    io_close_context(ctx);

    return 0;
}

int main(int argc, char **argv)
{
    printf("sizeof pointer=%ld enum=%ld\n", sizeof(uint32_t *), sizeof(PROXY_BUSY));
    printf("Create context\n");
    int i, ret;
    io_context *ctx = io_create_context(); //_net_context("192.168.2.5", 12345);
    printf("Add devices\n");

    io_stream_device *dma_tx = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_proxy_tx");

    int test_times = 2, j = 0;

    int test_size =  MAX_SAMPLES;
    for (int j = 0; j < test_times; j++)
    {
        struct channel_buffer *buffer_test = io_stream_get_buffer(dma_tx);

        for (i = 0; i < test_size; i++)
        {
            buffer_test->buffer[i].I0 = i + j * test_size;
            buffer_test->buffer[i].Q0 = i + j * test_size;
        }
        io_write_stream_device(dma_tx, buffer_test, test_size * sizeof(iq_buffer));
    }
    //    io_finish_stream_local(dma_tx);
    io_stream_device *dma_rx = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_proxy_rx");

    int rx_test_size = test_size; // 256 + 128;

    struct channel_buffer *buffer_rx_test = io_stream_get_buffer(dma_rx);
    memset(buffer_rx_test->buffer, -1, sizeof(iq_buffer) * rx_test_size);
    struct channel_buffer *buffer_rx_test_2 = io_stream_get_buffer(dma_rx);
    memset(buffer_rx_test_2->buffer, -1, sizeof(iq_buffer) * rx_test_size);

    io_write_stream_device(dma_rx, buffer_rx_test, rx_test_size * sizeof(iq_buffer));

    io_write_stream_device(dma_rx, buffer_rx_test_2, rx_test_size * sizeof(iq_buffer));

    io_finish_stream_local(dma_rx);
    for (i = 0; i < rx_test_size; i++)

    {
        // printf("%d;", *((int *)&buffer_rx_test->buffer[i]));
        printf("%d %d; ", buffer_rx_test->buffer[i].I0, buffer_rx_test->buffer[i].Q0);
    }
    printf("\n");

    for (i = 0; i < rx_test_size; i++)
    {
        // printf("%d;", *((int *)&buffer_rx_test_2->buffer[i]));
        printf("%d %d; ", buffer_rx_test_2->buffer[i].I0, buffer_rx_test_2->buffer[i].Q0);
    }
    printf("\n");

    // sleep(5);
    // for (i = 0; i < rx_test_size; i++)
    // {
    //     printf("%d %d;", buffer_rx_test->buffer[i].I0, buffer_rx_test->buffer[i].Q0);
    // }
    // printf("\n");

    io_close_context(ctx);

    return 0;
}
