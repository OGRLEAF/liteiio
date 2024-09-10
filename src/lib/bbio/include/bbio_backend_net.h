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

io_mapped_device *io_open_mapped_net(io_context *ctx, char *file_path, size_t size);