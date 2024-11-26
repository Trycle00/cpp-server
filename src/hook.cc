#include "hook.h"
#include "config.h"
#include "fd_manager.h"
#include "iomanager.h"
#include "log.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <sys/ioctl.h>

static auto g_logger                             = GET_LOGGER("system");

trycle::ConfigVar<int>::ptr s_tcp_timeout_ms_var = trycle::Config::lookUp("tcp.timeout.ms", 5000, "tcp timeout ms");

namespace trycle
{
static thread_local bool t_is_enable_hook;

// DO(bind)
// DO(listen)
//    DO(open)

#define HOOK_FUN(DO) \
    DO(sleep)        \
    DO(usleep)       \
    DO(nanosleep)    \
    DO(socket)       \
    DO(connect)      \
    DO(accept)       \
    DO(read)         \
    DO(readv)        \
    DO(recv)         \
    DO(recvfrom)     \
    DO(recvmsg)      \
    DO(write)        \
    DO(writev)       \
    DO(send)         \
    DO(sendto)       \
    DO(sendmsg)      \
    DO(close)        \
    DO(fcntl)        \
    DO(ioctl)        \
    DO(getsockopt)   \
    DO(setsockopt)

bool is_enable_hook()
{
    return t_is_enable_hook;
}

void set_enable_hook(bool val)
{
    t_is_enable_hook = val;
}

void init_hook()
{
    t_is_enable_hook = false;

#define LOAD_SHARED_FUN(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(LOAD_SHARED_FUN)
#undef LOAD_SHARED_FUN
    // #define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    //     HOOK_FUN(XX);
    // #undef XX
}

struct HookIniter
{
    HookIniter()
    {
        init_hook();

        s_tcp_timeout_ms_var->add_listener(
            [](const int64_t& old_val, const int64_t& new_val)
            {
                if (old_val == new_val)
                {
                    return;
                }
                s_tcp_timeout_ms_var->setVal(new_val);
            });
    }
};

static HookIniter hook_initer;

} // namespace trycle

struct TimeInfo
{
    bool cancelled = false;
};

template <typename OrignalFunc, typename... Args>
static ssize_t do_io(int fd, OrignalFunc func, const char* func_name, int32_t event, uint64_t timeout_so, Args&&... args)
{
    if (!trycle::is_enable_hook)
    {
        return func(fd, std::forward<Args>(args)...);
    }
    trycle::FdCtx::ptr fd_ctx = trycle::FdMgr::GetSingleton()->get(fd);
    if (!fd_ctx)
    {
        return func(fd, std::forward<Args>(args)...);
    }

    if (fd_ctx->isClosed())
    {
        errno = EBADF;
        return -1;
    }
    if (!fd_ctx->getIsSocket() || fd_ctx->getIsUserNoBlock())
    {
        return func(fd, std::forward<Args>(args)...);
    }

    uint64_t to = fd_ctx->getTimeout(fd);
    std::shared_ptr<TimeInfo> time_info(new TimeInfo());

RETRY:
    ssize_t n = func(fd, std::forward<Args>(args)...);
    // 出现 EINTR，是因为系统 API 阻塞等待状态下被其它系统信号中断
    // 此处的解决办法是重新调用这次系统的 API
    while (n == -1 && errno == EINTR)
    {
        n = func(fd, std::forward<Args>(args)...);
    }

    // 出现 EAGAIN 是因为长时间未读到数据或无法写入数据
    // 直接把这个 fd 丢到 IOManager 里监听对应事件，触发后返回本执行上下文
    if (n == -1 && errno == EAGAIN)
    {
        trycle::IOManager* iom = trycle::IOManager::GetThis();
        trycle::Timer::ptr timer;
        if (to != (uint64_t)-1)
        {
            std::weak_ptr<TimeInfo> wtime_info(time_info);
            timer = iom->addConditionTimer(
                timeout_so,
                [fd, event, iom, wtime_info]()
                {
                    auto tinfo = wtime_info.lock();
                    if (!tinfo || tinfo->cancelled)
                    {
                        return;
                    }
                    tinfo->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (trycle::IOManager::EventType)event);
                },
                wtime_info, false);
        }

        int rt = iom->addEvent(fd, (trycle::IOManager::EventType)event);
        if (rt)
        {
            LOG_FMT_ERROR(g_logger, "addEvent failed | func_name=%s, fd=%d", func_name, fd);
            if (timer)
            {
                timer->cancel();
            }
            return -1;
        }

        trycle::Fiber::YieldToHold();
        if (timer)
        {
            timer->cancel();
        }
        if (time_info->cancelled)
        {
            errno = time_info->cancelled;
            return -1;
        }

        goto RETRY;
    }

    return n;
}

extern "C"
{
#define DEFINE_FUN(name) name##_fun name##_f = nullptr;
    HOOK_FUN(DEFINE_FUN)
#undef DEFINE_FUN

    unsigned int sleep(unsigned int seconds)
    {
        if (!trycle::t_is_enable_hook)
        {
            return sleep_f(seconds);
        }

        trycle::Fiber::ptr fiber = trycle::Fiber::GetThis();
        trycle::IOManager* iom   = trycle::IOManager::GetThis();

        // iom->addTimer(
        //     seconds * 1000,
        //     [fiber, iom]()
        //     {
        //         iom->schedule(fiber);
        //     },
        //     false);
        iom->addTimer(
            seconds * 1000,
            std::bind((void(trycle::Scheduler::*)(trycle::Fiber::ptr, int)) & trycle::IOManager::schedule, iom, fiber, -1),
            false);

        trycle::Fiber::YieldToHold();

        return 0;
    }

    int usleep(useconds_t usec)
    {
        if (!trycle::t_is_enable_hook)
        {
            return usleep_f(usec);
        }
        trycle::Fiber::ptr fiber = trycle::Fiber::GetThis();
        trycle::IOManager* iom   = trycle::IOManager::GetThis();
        // iom->addTimer(
        //     usec / 1000,
        //     [fiber, iom]()
        //     { iom->schedule(fiber); },
        //     false);
        iom->addTimer(
            usec / 1000,
            std::bind((void(trycle::Scheduler::*)(trycle::Fiber::ptr, int)) & trycle::IOManager::schedule, iom, fiber, -1),
            false);
        trycle::Fiber::YieldToHold();

        return 0;
    }

    int nanosleep(const struct timespec* req, struct timespec* rem)
    {
        if (!trycle::t_is_enable_hook)
        {
            return nanosleep_f(req, rem);
        }

        trycle::Fiber::ptr fiber = trycle::Fiber::GetThis();
        trycle::IOManager* iom   = trycle::IOManager::GetThis();

        uint64_t timeout_ms      = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        // iom->addTimer(
        //     timeout_ms,
        //     [fiber, iom]()
        //     { iom->schedule(fiber); },
        //     false);
        iom->addTimer(
            timeout_ms,
            std::bind((void(trycle::Scheduler::*)(trycle::Fiber::ptr, int)) & trycle::IOManager::schedule, iom, fiber, -1),
            false);
        trycle::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol)
    {
        if (!trycle::t_is_enable_hook)
        {
            return socket_f(domain, type, protocol);
        }

        int fd = socket_f(domain, type, protocol);

        if (fd == -1)
        {
            return fd;
        }
        trycle::FdMgr::GetSingleton()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
    {
        if (!trycle::t_is_enable_hook)
        {
            return connect_f(sockfd, addr, addrlen);
        }
        auto ctx = trycle::FdMgr::GetSingleton()->get(sockfd);
        if (!ctx || ctx->isClosed())
        {
            errno = EBADF;
            return -1;
        }
        if (!ctx->getIsSocket() || ctx->getIsUserNoBlock())
        {
            return connect_f(sockfd, addr, addrlen);
        }

        int n = connect_f(sockfd, addr, addrlen);
        if (n == 0)
        {
            return 0;
        }
        else if (n != -1 || errno != EINPROGRESS)
        {
            return n;
        }

        trycle::IOManager* iom = trycle::IOManager::GetThis();
        trycle::Timer::ptr timer;
        std::shared_ptr<TimeInfo> tinfo(new TimeInfo());
        std::weak_ptr<TimeInfo> wtinfo(tinfo);

        if (timeout_ms == (uint64_t)-1)
        {
            timer = iom->addConditionTimer(
                timeout_ms,
                [sockfd, iom, wtinfo]()
                {
                    auto t = wtinfo.lock();
                    if (!t || t->cancelled)
                    {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(sockfd, trycle::IOManager::EventType::WRITE);
                },
                wtinfo, false);
        }

        int rt = iom->addEvent(sockfd, trycle::IOManager::EventType::WRITE);
        if (rt == 0)
        {
            trycle::Fiber::YieldToHold();
            if (timer)
            {
                timer->cancel();
            }
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else
        {
            if (timer)
            {
                timer->cancel();
            }
            LOG_FMT_ERROR(g_logger, "connect addEvent failed | WRITE, fd=%d", sockfd);
        }

        int error     = 0;
        socklen_t len = sizeof(int);
        if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len))
        {
            LOG_FMT_ERROR(g_logger, "connect getsockopt failed | fd=%d", sockfd);
            return -1;
        }
        if (!error)
        {
            return 0;
        }

        errno = error;
        return -1;
    }

    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
    {
        // if (!trycle::t_is_enable_hook)
        // {
        //     return connect_f(sockfd, addr, addrlen);
        // }
        // if (sockfd > 0)
        // {
        //     trycle::FdMgr::GetSingleton()->get(sockfd, true);
        // }
        return connect_with_timeout(sockfd, addr, addrlen, s_tcp_timeout_ms_var->getVal());
    }

    int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
    {
        if (!trycle::t_is_enable_hook)
        {
            return accept_f(sockfd, addr, addrlen);
        }
        if (sockfd > 0)
        {
            trycle::FdMgr::GetSingleton()->get(sockfd, true);
        }
        return sockfd;
    }

    ssize_t read(int fd, void* buf, size_t count)
    {
        return do_io(fd, read_f, "read", trycle::IOManager::EventType::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec* iov, int iovcnt)
    {
        return do_io(fd, readv_f, "readv", trycle::IOManager::EventType::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void* buf, size_t len, int flags)
    {
        return do_io(sockfd, recv_f, "recv", trycle::IOManager::EventType::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
    {
        return do_io(sockfd, recvfrom_f, "recvfrom", trycle::IOManager::EventType::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags)
    {
        return do_io(sockfd, recvmsg_f, "recvmsg", trycle::IOManager::EventType::READ, SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void* buf, size_t count)
    {
        return do_io(fd, write_f, "write", trycle::IOManager::EventType::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
    {
        return do_io(fd, writev_f, "writev", trycle::IOManager::EventType::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int sockfd, const void* buf, size_t len, int flags)
    {
        return do_io(sockfd, send_f, "send", trycle::IOManager::EventType::WRITE, SO_SNDTIMEO, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen)
    {
        return do_io(sockfd, sendto_f, "sendto", trycle::IOManager::EventType::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags)
    {
        return do_io(sockfd, sendmsg_f, "sendmsg", trycle::IOManager::EventType::WRITE, SO_SNDTIMEO, msg, flags);
    }

    int close(int fd)
    {
        if (!trycle::t_is_enable_hook)
        {
            return close_f(fd);
        }
        auto fd_ctx = trycle::FdMgr::GetSingleton()->get(fd);
        if (fd_ctx)
        {
            auto iom = trycle::IOManager::GetThis();
            if (iom)
            {
                iom->cancelAllEvent(fd);
            }
            trycle::FdMgr::GetSingleton()->del(fd);
        }
        return close_f(fd);
    }

    int fcntl(int fd, int cmd, ... /* arg */)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
            case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                auto ctx = trycle::FdMgr::GetSingleton()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->getIsSocket())
                {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setIsUserNoBlock(arg & O_NONBLOCK);
                if (ctx->getIsSysNoBlock())
                {
                    arg |= O_NONBLOCK;
                }
                else
                {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            case F_GETFL:
            {
                va_end(va);
                // int arg  = va_arg(va, int);
                int arg  = fcntl_f(fd, cmd);
                auto ctx = trycle::FdMgr::GetSingleton()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->getIsSocket())
                {
                    return arg;
                }
                if (ctx->getIsUserNoBlock())
                {
                    return arg | O_NONBLOCK;
                }
                else
                {
                    return arg & ~O_NONBLOCK;
                }
            }

            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
#ifdef F_SETPIPE_SZ
            case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;

            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
#ifndef F_GETPIPE_SZ
            case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;

            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;

            case F_SETOWN_EX:
            case F_GETOWN_EX:
            {
                struct f_owner_ex* arg = va_arg(va, struct f_owner_ex*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;

            default:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        }
        return 0;
    }

    int ioctl(int fd, unsigned long request, ...)
    {
        va_list va;
        va_start(va, request);
        void* arg = va_arg(va, void*);
        va_end(va);

        if (FIONBIO == request)
        {
            bool userNonblock         = !!*(int*)arg;
            trycle::FdCtx::ptr fd_ctx = trycle::FdMgr::GetSingleton()->get(fd);
            if (!fd_ctx || fd_ctx->isClosed() || !fd_ctx->getIsSocket())
            {
                return ioctl_f(fd, request, arg);
            }
            fd_ctx->setIsUserNoBlock(userNonblock);
        }

        return ioctl_f(fd, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
    {
        return do_io(sockfd, getsockopt_f, "getsockopt", trycle::IOManager::EventType::READ, SO_RCVTIMEO, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
    {
        if (!trycle::t_is_enable_hook)
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                auto fd_ctx = trycle::FdMgr::GetSingleton()->get(sockfd);
                if (fd_ctx)
                {
                    const timeval* tval = (const timeval*)optval;
                    uint64_t timeout_ms = tval->tv_sec * 1000 + tval->tv_usec / 1000;
                    fd_ctx->setTimeout(optname, timeout_ms);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}
