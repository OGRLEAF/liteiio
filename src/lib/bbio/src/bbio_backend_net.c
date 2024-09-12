#include "bbio_h.h"
#include "bbio_private_h.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <sys/uio.h>
#include <time.h>

#include "bbio_backend.h"
#include "bbio_backend_net.h"

#define SOCK_FLAGS 0 //(MSG_ZEROCOPY)

handshake_msg mapped_handshake_msg = {
    .head = HANDSHAKE_HEADER,
    .type = 0,
};

handshake_msg stream_handshake_msg = {
    .head = HANDSHAKE_HEADER,
    .type = 1,
};

typedef struct _open_call_param
{
    uint8_t file_path_len;
    int flag;
} open_call_param;

void io_make_net_call(io_net_context *ctx, enum cmd_call_type type, uint16_t size, char *content)
{
}

int io_net_call_create(io_net_context *ctx, enum cmd_call_type type)
{
    int sockfd = ctx->sockfd;
    uint8_t cmd_type = 0;
    uint8_t type_value = ((uint8_t)type << 1) | (cmd_type & 0b1);
    int n;
    printf("Call %d/%d\n", cmd_type, type);
    n = send(sockfd, &type_value, sizeof(type_value), 0);
    return n;
}
// #define _DEBUG
#ifdef _DEBUG
#define _call_print(...) printf(__VA_ARGS__)
#else
#define _call_print(...)
#endif

#define io_net_call_start(ctx, type)                                \
    {                                                               \
        uint8_t type_value = ((uint8_t)type << 1) | (0 & 0b1);      \
        _call_print("Call %s %d\n", #type, type);                   \
        send(ctx->fd, &type_value, sizeof(type_value), SOCK_FLAGS); \
    }
#define io_net_call_param(ctx, size, value)                  \
    {                                                        \
        _call_print("    \tparam len=%d %p\n", size, value); \
        send(ctx->fd, value, size, SOCK_FLAGS);              \
    }

#define io_net_call_startv(ctx, iov, type)                     \
    {                                                          \
        uint8_t type_value = ((uint8_t)type << 1) | (0 & 0b1); \
        _call_print("Call %s %d\n", #type, type);              \
        iov.iov_base = &type_value;                            \
        iov.iov_len = sizeof(uint8_t);                         \
    }
#define io_net_call_paramv(ctx, iov, size, value)            \
    {                                                        \
        _call_print("    \tparam len=%d %p\n", size, value); \
        iov.iov_base = value;                                \
        iov.iov_len = size;                                  \
    }

#define io_net_call_return_value(ctx, size, ret) \
    {                                            \
        recv(ctx->fd, ret, size, 0);             \
    }
#define io_net_call_return_null(ctx) \
    {                                \
    }
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define io_net_call_return(...) GET_MACRO(__VA_ARGS__, io_net_call_return_value, _, io_net_call_return_null)(__VA_ARGS__)

void io_write_net(io_mapped_device *device, uint32_t addr, uint32_t value)
{
    // io_net_context *ctx_net = (io_net_context *)ctx;
    io_net_call_start(device, CALL_WRITE);
    io_net_call_param(device, sizeof(addr), &addr);
    io_net_call_param(device, sizeof(value), &value);
    io_net_call_return(device);
    // io_net_call_return(ctx_net, sizeof(rptr), &rptr);
}

uint32_t io_read_net(io_mapped_device *device, uint32_t addr)
{
    uint32_t value;
    io_net_call_start(device, CALL_READ);
    io_net_call_param(device, sizeof(addr), &addr);
    io_net_call_return(device, sizeof(value), &value);
    return value;
}

static struct channel_buffer *io_stream_alloc_buffer_net(io_stream_device *device, uint32_t flag)
{
    // printf("get buffer\n");
    return (struct channel_buffer *)device->ch.private;
}

static uint32_t io_write_stream_net(io_stream_device *device, void *data, uint32_t size)
{
    int flag_on = 1, flag_off = 0;
    struct iovec iov[3];
    // io_net_call_startv(device, iov[0], CALL_WRITE_STREAM);
    // io_net_call_paramv(device, iov[1], sizeof(size), &size);
    // io_net_call_paramv(device, iov[2], size, data);
    // writev(device->fd, iov, 3);
    io_net_call_start(device, CALL_WRITE_STREAM);
    io_net_call_param(device, sizeof(size), &size);
    io_net_call_param(device, size, data);
    // io_net_call_param(device, size, data);
    // io_net_call_return(device, sizeof(size), &size);
    return size;
}

static int io_handshake(int sockfd, enum cmd_channel_type type)
{
    handshake_msg msg = {
        .head = HANDSHAKE_HEADER,
        .type = type,
    };
    uint32_t ack = 0;
    int n;

    send(sockfd, &msg, sizeof(msg), SOCK_FLAGS);
    n = recv(sockfd, &ack, sizeof(ack), SOCK_FLAGS);
    if ((n <= 0) || (ack != HANDSHAKE_HEADER))
        return -1;
    printf("Handshake ok\n");
    return 0;
}

static IO_FD io_open_remote_device(int sockfd, char *dev_path, size_t size)
{
    uint32_t path_len;
    IO_FD fd;
    int n;
    // open device
    path_len = strlen(dev_path);
    printf("open remote %d %s %ld\n", path_len, dev_path, size);
    send(sockfd, &path_len, sizeof(path_len), 0);
    send(sockfd, dev_path, path_len, 0);
    send(sockfd, &size, sizeof(size), 0);
    n = recv(sockfd, &fd, sizeof(fd), 0);
    if (n <= 0 || (fd <= 0))
        return -1;

    return fd;
}
io_mapped_device *io_open_mapped_net(io_context *ctx, char *file_path, size_t size)
{
    int sockfd, remotefd;
    int ret;
    const int flag = 1;
    io_net_context *ctx_net = (io_net_context *)ctx;
    io_mapped_device *device;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        return NULL;

    ret = connect(sockfd, (struct sockaddr *)&ctx_net->serv_addr, sizeof(ctx_net->serv_addr));

    if (ret < 0)
    {
        printf("Server connect failed.\n");
        return NULL;
    }
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &flag, sizeof(flag));

    // handshake
    printf("Handshake...\n");
    if (io_handshake(sockfd, MAPPED_CHANNEL))
        return NULL;
    // send file info
    remotefd = io_open_remote_device(sockfd, file_path, size);

    device = (io_mapped_device *)malloc(sizeof(io_mapped_device));
    device->fd = sockfd;

    device->ch.write = io_write_net;
    device->ch.read = io_read_net;

    return device;
}

io_stream_device *io_open_stream_net(io_context *ctx, char *file_path)
{
    int sockfd, remotefd;
    int ret;
    const int flag = 1;
    io_net_context *ctx_net = (io_net_context *)ctx;
    io_stream_device *device;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        return NULL;

    printf("Connect stream for device %s\n", file_path);
    ret = connect(sockfd, (struct sockaddr *)&ctx_net->serv_addr, sizeof(ctx_net->serv_addr));

    if (ret < 0)
    {
        printf("Server connect failed.\n");
        return NULL;
    }
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &flag, sizeof(flag));

    // handshake
    if (io_handshake(sockfd, STREAM_CHANNEL))
        return NULL;
    // send file info
    printf("Open remote...");
    remotefd = io_open_remote_device(sockfd, file_path, 0);

    if (remotefd < 1)
        return NULL;
    printf("Remote device %s open ok\n", file_path);

    device = (io_stream_device *)malloc(sizeof(io_stream_device));
    device->fd = sockfd;

    device->ch.write_stream = io_write_stream_net;
    device->ch.private = (void *)malloc(sizeof(struct channel_buffer));
    device->ch.alloc_buffer = io_stream_alloc_buffer_net;

    setsockopt(device->fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof(flag));
    io_net_call_start(device, CALL_WRITE_STREAM);

    return device;
}