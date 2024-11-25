#ifndef TRY_HOOK_H
#define TRY_HOOK_H

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <timer.h>
#include <unistd.h>

namespace trycle
{

bool is_enable_hook();
void set_enable_hook(bool val);

} // namespace trycle

extern "C"
{

    // sleep
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun usleep_f;

    typedef int (*nanosleep_fun)(const struct timespec* req, struct timespec* rem);
    extern nanosleep_fun nanosleep_f;

    // socket
    typedef int (*socket_fun)(int domain, int type, int protocol);
    extern socket_fun socket_f;

    typedef int (*bind_fun)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    extern bind_fun bind_f;

    typedef int (*listen_fun)(int sockfd, int backlog);
    extern listen_fun listen_f;

    typedef int (*connect_fun)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    extern connect_fun connect_f;

    typedef int (*accept_fun)(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
    extern accept_fun accept_f;

    // read
    typedef ssize_t (*read_fun)(int fd, void* buf, size_t count);
    extern read_fun read_f;

    typedef ssize_t (*readv_fun)(int fd, const struct iovec* iov, int iovcnt);
    extern readv_fun readv_f;

    typedef ssize_t (*recv_fun)(int sockfd, void* buf, size_t len, int flags);
    extern recv_fun recv_f;

    typedef ssize_t (*recvfrom_fun)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
    extern recvfrom_fun recvfrom_f;

    typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr* msg, int flags);
    extern recvmsg_fun recvmsg_f;

    // write
    typedef ssize_t (*write_fun)(int fd, const void* buf, size_t count);
    extern write_fun write_f;

    typedef ssize_t (*writev_fun)(int fd, const struct iovec* iov, int iovcnt);
    extern writev_fun writev_f;

    typedef ssize_t (*send_fun)(int sockfd, const void* buf, size_t len, int flags);
    extern send_fun send_f;

    typedef ssize_t (*sendto_fun)(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen);
    extern sendto_fun sendto_f;

    typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr* msg, int flags);
    extern sendmsg_fun sendmsg_f;

    // socket ctl
    typedef int (*close_fun)(int fd);
    extern close_fun close_f;

    typedef int (*open_fun)(const char* pathname, int flags);
    extern open_fun open_f;

    typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
    extern fcntl_fun fcntl_f;
}

#endif // TRY_HOOK_H
