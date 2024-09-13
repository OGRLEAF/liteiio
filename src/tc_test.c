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

int main(int argc, char **argv)
{
    printf("sizeof pointer=%ld enum=%ld\n", sizeof(uint32_t *), sizeof(PROXY_BUSY));
    printf("Create context\n");
    int i, ret;
    io_context *ctx = io_create_context(); //_net_context("192.168.2.5", 12345);
    printf("Add devices\n");
    io_mapped_device *dev = (io_mapped_device *)io_add_mapped_device(ctx, "/dev/tc");

    io_write_mapped_device(dev, 0, 1234);

    int read_value = io_read_mapped_device(dev, 0);
    printf("Read value=%d\n", read_value);

    io_stream_device *dma_dev = (io_stream_device *)io_add_stream_device(ctx, "/dev/dma_proxy_tx");

    int test_times = 64, j = 0;
    for (int j = 0; j < test_times ; j++)
    {
        struct channel_buffer *buffer_test = io_stream_get_buffer(dma_dev);

        int test_size = MAX_SAMPLES;
        for (i = 0; i < test_size; i++)
        {
            buffer_test->buffer[i].I0 = i + j * test_size;
            buffer_test->buffer[i].Q0 = i + j * test_size;
        }

        // printf("Sync buffer samples=%d buffer=%p\n", test_size, buffer_test);

        io_write_stream_device(dma_dev, buffer_test, test_size * sizeof(iq_buffer));
    }
    io_close_context(ctx);

    return 0;
}