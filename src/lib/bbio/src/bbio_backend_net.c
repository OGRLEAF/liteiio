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

#define io_net_call_start(ctx, type)                                    \
    {                                                                   \
        uint8_t type_value = ((uint8_t)type << 1) | (0 & 0b1);          \
        _call_print("Call %s %d\n", #type, type);                       \
        send(ctx->fd, &type_value, sizeof(type_value), SOCK_FLAGS); \
    }
#define io_net_call_param(ctx, size, value)                  \
    {                                                        \
        _call_print("    \tparam len=%d %p\n", size, value); \
        send(ctx->fd, value, size, SOCK_FLAGS);          \
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
// int io_net_call_param(io_net_context *ctx, size_t size, void *value)
// {
//     int n;
//     printf("    \tparam len=%d\n", size);
//     n = send(ctx->sockfd, value, size, 0);
//     return n;
// }
#define io_net_call_return_value(ctx, size, ret) \
    {                                            \
        recv(ctx->fd, ret, size, 0);         \
    }
#define io_net_call_return_null(ctx) \
    {                                \
    }
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define io_net_call_return(...) GET_MACRO(__VA_ARGS__, io_net_call_return_value, _, io_net_call_return_null)(__VA_ARGS__)

// int io_net_call_return(io_net_context *ctx, size_t size, void *ret)
// {
//     int n;
//     n = recv(ctx->sockfd, ret, size, 0);
// }

// IO_FD io_open_net(io_context *ctx, char *file, int flag)
// {
//     uint8_t file_path_len = strlen(file);
//     IO_FD fd;
//     io_net_context *ctx_net = (io_net_context *)ctx;

//     io_net_call_start(ctx_net, CALL_OPEN);
//     io_net_call_param(ctx_net, sizeof(file_path_len), &file_path_len);
//     io_net_call_param(ctx_net, file_path_len, file);
//     io_net_call_param(ctx_net, sizeof(flag), &flag);
//     io_net_call_return(ctx_net, sizeof(fd), &fd);

//     return fd;
// }

// // int io_close_local(io_context *ctx, IO_FD fd)
// // {
// //     return close(fd);
// // }

// void *io_mmap_net(io_context *ctx, void **addr, IO_FD fd, size_t __len)
// {
//     io_net_context *ctx_net = (io_net_context *)ctx;
//     void *rptr = NULL;
//     // void *local_mirror'
//     printf("map net : fd=%d len=%ld\n", fd, __len);
//     io_net_call_start(ctx_net, CALL_MMAP);
//     io_net_call_param(ctx_net, sizeof(IO_FD), &fd);
//     io_net_call_param(ctx_net, sizeof(__len), &__len);
//     io_net_call_return(ctx_net, sizeof(rptr), &rptr);

//     if (rptr)
//     {
//         *addr = rptr;
//     }

//     return rptr;
// }

// int io_ioctl_net(io_context *ctx, int __fd, unsigned long __request, void *payload)
// {
//     io_net_context *ctx_net = (io_net_context *)ctx;
//     int ret;
//     // void *local_mirror'
//     io_net_call_start(ctx_net, CALL_IOCTL);
//     io_net_call_param(ctx_net, sizeof(IO_FD), &__fd);
//     io_net_call_param(ctx_net, sizeof(__request), &__request);
//     io_net_call_param(ctx_net, sizeof(int), payload);
//     io_net_call_return(ctx_net, sizeof(ret), &ret);
//     return ret;
// }

// uint16_t io_write_busrt_net(io_context *ctx, void *addr, void *data, uint32_t size)
// {
//     io_net_context *ctx_net = (io_net_context *)ctx;
//     int flag_on = 1, flag_off = 0;
//     struct iovec iov[4];
//     io_net_call_startv(ctx_net, iov[0], CALL_WRITE_BURST);
//     io_net_call_paramv(ctx_net, iov[1], sizeof(addr), &addr);
//     io_net_call_paramv(ctx_net, iov[2], sizeof(size), &size);
//     io_net_call_paramv(ctx_net, iov[3], size, data);
//     setsockopt(ctx_net->sockfd, IPPROTO_TCP, TCP_CORK, &flag_on, sizeof(flag_on));
//     writev(ctx_net->sockfd, iov, 4);
//     setsockopt(ctx_net->sockfd, IPPROTO_TCP, TCP_CORK, &flag_off, sizeof(flag_off));
//     io_net_call_return(ctx_net, sizeof(size), &size);
//     return size;
// }

// uint16_t io_read_busrt_net(io_context *ctx, void *addr, void *data, uint32_t size)
// {
//     io_net_context *ctx_net = (io_net_context *)ctx;
//     io_net_call_start(ctx_net, CALL_WRITE_BURST);
//     io_net_call_param(ctx_net, sizeof(addr), &addr);
//     io_net_call_param(ctx_net, sizeof(size), &size);
//     io_net_call_return(ctx_net, size, data);
//     return size;
// }

void io_sync_buffer_local(io_context *ctx, uint32_t *addr, void *data, int16_t size, uint8_t dir)
{
    io_write_busrt_net(ctx, addr, data, size);
}

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


static uint16_t io_write_stream_net(io_stream_device *device, void *data, uint32_t size)
{
    int flag_on = 1, flag_off = 0;
    struct iovec iov[4];
    io_net_call_startv(device, iov[0], CALL_WRITE_BURST);
    io_net_call_paramv(device, iov[2], sizeof(size), &size);
    io_net_call_paramv(device, iov[3], size, data);
    setsockopt(device->fd, IPPROTO_TCP, TCP_CORK, &flag_on, sizeof(flag_on));
    writev(device->fd, iov, 4);
    setsockopt(device->fd, IPPROTO_TCP, TCP_CORK, &flag_off, sizeof(flag_off));
    io_net_call_return(device, sizeof(size), &size);
    return size;
}

io_mapped_device *io_open_mapped_net(io_context *ctx, char *file_path, size_t size)
{
    int sockfd;
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

    device = (io_mapped_device *)malloc(sizeof(io_mapped_device));
    device->fd = sockfd;

    device->ch.write = io_write_net;
    device->ch.read = io_read_net;

    return device;
}



io_stream_device *io_open_stream_net(io_context *ctx, char *file_path, size_t size)
{
    int sockfd;
    int ret;
    const int flag = 1;
    io_net_context *ctx_net = (io_net_context *)ctx;
    io_stream_device *device;
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


    device = (io_mapped_device *)malloc(sizeof(io_mapped_device));
    device->fd = sockfd;

    device->ch.write_stream = io_write_stream_net;

    return device;
}