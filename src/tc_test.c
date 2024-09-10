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

    struct channel_buffer *buffer_test = (struct channel_buffer *)malloc(sizeof(struct channel_buffer));

    int test_size = MAX_SAMPLES;
    for (i = 0; i < test_size; i++)
    {
        buffer_test->buffer[i].I0 = i;
        buffer_test->buffer[i].Q0 = i;
    }

    printf("Sync buffer samples=%d\n", test_size);

    io_write_stream_device(dma_dev, buffer_test, test_size * sizeof(iq_buffer));
    io_write_stream_device(dma_dev, buffer_test, test_size * sizeof(iq_buffer));

    return 0;
}