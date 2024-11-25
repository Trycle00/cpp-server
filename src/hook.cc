#include "hook.h"
#include "fd_manager.h"
#include "iomanager.h"
#include "log.h"

#include <arpa/inet.h>
#include <dlfcn.h>

namespace trycle
{
static thread_local bool t_is_enable_hook;
static auto g_logger = GET_LOGGER("system");

#define HOOK_FUN(DO) \
    DO(sleep)        \
    DO(usleep)       \
    DO(nanosleep)    \
    DO(socket)       \
    DO(bind)         \
    DO(listen)       \
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
    DO(open)         \
    DO(fcntl)

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
    }
};

static HookIniter hook_initer;

} // namespace trycle

struct TimeInfo
{
    bool cancelled = false;
};

template <typename OrignalFunc, typename... Args>
static ssize_t do_io(int fd, OrignalFunc func, char* func_name, int32_t event, uint64_t timeout_so, Args&&... args)
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
                wtime_info);
        }

        int rt = iom->addEvent(fd, (trycle::IOManager::EventType)event);
        if (rt)
        {
            LOG_FMT_ERROR(g_logger, "addEvent failed | func_name=%s, fd=%ld", func_name, fd);
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
    // #define XX(name) name##_fun name##_f = nullptr;
    //     HOOK_FUN(XX);
    // #undef XX

    unsigned int sleep(unsigned int seconds)
    {
        if (!trycle::t_is_enable_hook)
        {
            return sleep_f(seconds);
        }

        trycle::Fiber::ptr fiber = trycle::Fiber::GetThis();
        trycle::IOManager* iom   = trycle::IOManager::GetThis();

        iom->addTimer(seconds * 1000, [fiber, iom]()
                      { iom->schedule(fiber); }, false);

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
        iom->addTimer(usec / 1000, [fiber, iom]()
                      { iom->schedule(fiber); }, false);
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

        uint64_t ms              = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        iom->addTimer(ms, [fiber, iom]()
                      { iom->schedule(fiber); }, false);
        trycle::Fiber::YieldToHold();
        return 0;
    }
}
