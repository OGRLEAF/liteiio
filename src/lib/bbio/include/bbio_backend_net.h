#include "bbio_backend.h"

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>

enum cmd_channel_type
{
    MAPPED_CHANNEL = 0,
    STREAM_CHANNEL
};

enum cmd_call_type
{
    CALL_OPEN = 0,
    CALL_CLOSE = 1,
    CALL_MMAP,
    CALL_IOCTL,
    CALL_WRITE,
    CALL_READ,
    CALL_WRITE_BURST,
    CALL_READ_BURST,
};

typedef struct _bbio_net_context
{
    io_context context;
    struct sockaddr_in serv_addr;
    int sockfd;
    pthread_mutex_t lock;
} io_net_context;

IO_FD io_open_net(io_context *ctx, char *file, int flag);
void *io_mmap_net(io_context *ctx, void **addr, IO_FD fd, size_t __len);
int io_ioctl_net(io_context *ctx, int __fd, unsigned long __request, void *payload);

void io_write_net(io_context *ctx, uint32_t *addr, uint32_t value);
uint32_t io_read_net(io_context *ctx, uint32_t *addr);

uint16_t io_read_busrt_net(io_context *ctx, void *addr, void *data, uint32_t size);
uint16_t io_write_busrt_net(io_context *ctx, void *addr, void *data, uint32_t size);

io_mapped_device *io_open_mapped_net(io_context *ctx, char *file_path, size_t size);