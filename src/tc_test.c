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
#include <poll.h>
#include "bbio_h.h"

#define TEST_SIZE 512

static int stop = 0;
void io_finish_stream_local(io_stream_device *device);

int main_(int argc, char **argv)
{
    printf("sizeof pointer=%ld enum=%ld\n", sizeof(uint32_t *), sizeof(PROXY_BUSY));
    printf("Create context\n");
    int i, ret;
    io_context *ctx = io_create_context(); //_net_context("192.168.2.5", 12345);
    printf("Add devices\n");

    io_stream_device *dma_tx = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_proxy_tx");

    int test_times = 4, j = 0;

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

int validate_data(struct channel_buffer *buffer_rx, int test_size, int j)
{
    int i;
    int check_value;
    int err = 0;
    for (i = 0; i < test_size; i++)
    {
        int value = *((int *)&buffer_rx->buffer[i]);
        // printf("%d ", value);
        check_value = i << 1;
        if (value == check_value)
            continue;
        err = 1;
        printf("Data validate error at %d - [%d]:  Expect %d, recieve %d \n", j, i + 1, check_value, value);
        break;
        // return 0;
    }

    if (!err)
    {
        printf("Buffer %d len=%d Validate ok\n", j, test_size);
    }
    else
    {
        for (i = 0; i < test_size; i++)
        {
            int value = *((int *)&buffer_rx->buffer[i]);
            printf("%d ", value);
        }
        printf("\n");
    }
}

void sigint(int a)
{
    printf("Stop transmit...\n");
    stop = 1;
}

int main(int argc, char **argv)
{
    int i, ret;
    io_context *ctx = io_create_context(); //_net_context("192.168.2.5", 12345);

    
    printf("sizeof pointer=%ld enum=%ld\n", sizeof(uint32_t *), sizeof(PROXY_BUSY));
    printf("Create context\n");


    printf("Add devices\n");

    io_mapped_device *map_dev = (io_mapped_device *)io_add_mapped_device(ctx, "/dev/tc");
    int test_buffers = 32, j = 0;
    struct channel_buffer *buffers_rx_test[32];
    int test_size = 32768, loop_times = 100000;

    io_stream_device *dma_rx = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_mm_rx");

    int rx_test_size = test_size; // 256 + 128;

    // perform reset1
    io_write_mapped_device(map_dev, 5, 3);

    // configure stream size
    io_write_mapped_device(map_dev, 6, test_size);

    // release reset
    io_write_mapped_device(map_dev, 5, 0);

    signal(SIGINT, sigint);
    while (loop_times && !stop)
    {
        for (j = 0; (j < test_buffers) && !stop; j++)
        {

            struct channel_buffer *buffer_rx_test = io_stream_get_buffer(dma_rx);
            buffers_rx_test[j % 32] = buffer_rx_test;
            // memset(buffer_rx_test->buffer, -1, sizeof(iq_buffer) * rx_test_size);
            io_read_stream_device(dma_rx, buffer_rx_test, rx_test_size * sizeof(iq_buffer));
            // io_finish_stream_local(dma_rx);
            if (!buffer_rx_test)
            {
                printf("Buffer request timeout\n");
                goto exit;
            }
        }

        loop_times--;
        // io_finish_stream_local(dma_rx);
        // for (j = 0; (j < test_buffers) && !stop; j++)
        // {
        //     validate_data(buffers_rx_test[j % 32], test_size, j);
        // }
    }
exit:
    io_finish_stream_local(dma_rx);
    io_write_mapped_device(map_dev, 5, 3);

    io_close_context(ctx);

    return 0;
}
