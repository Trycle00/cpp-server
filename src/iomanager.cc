#include "iomanager.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "log.h"
#include "macro.h"

namespace trycle
{

static auto g_logger = GET_LOGGER("system");

/**
 * ============================================================================
 * IOManager 类的实现
 * ============================================================================
 */
IOManager::IOManager(size_t thread_size, bool use_caller, const std::string& name)
    : Scheduler(thread_size, use_caller, name)
{
    // 创建epoll
    m_epoll_fd = ::epoll_create(0xffff);
    ASSERT(m_epoll_fd > 0);
    // 创建管道，并加入epoll监听
    int pip_res = ::pipe(m_tickle_fds);
    ASSERT(!pip_res);
    // 创建管理可读事件监听
    epoll_event event{};
    event.data.fd = m_tickle_fds[0];
    // 开启可读事件，并开启边缘触发
    event.events = EPOLLIN | EPOLLET;
    // 将管道读取端设置为非阻塞模式
    int fcntl_res = ::fcntl(m_tickle_fds[0], F_SETFL, O_NONBLOCK);
    ASSERT(!fcntl_res);

    int ep_ctl_res = ::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_tickle_fds[0], &event);
    ASSERT(!ep_ctl_res);

    contextListResize(32);

    start();
}

IOManager::~IOManager()
{
    // 触用 stop
    stop();
    // 关闭打开的文件标识符
    close(m_epoll_fd);
    close(m_tickle_fds[0]);
    close(m_tickle_fds[1]);
}

void IOManager::contextListResize(int size)
{
    m_fd_context_list.resize(size);
    for (int i = 0; i < m_fd_context_list.size(); i++)
    {
        if (!m_fd_context_list[i])
        {
            m_fd_context_list[i]       = std::make_shared<FdContext>();
            m_fd_context_list[i]->m_fd = i;
        }
    }
}

void IOManager::onTimerInsertedAtFirst()
{
    tickle();
}

/**
 * return 0 as success
 */
bool IOManager::addEvent(int fd, EventType event, std::function<void()> callback)
{
    /**
     * NOTE:
     *  主要工作流程：
     *  首先，从fd对象池中取出对应的对象指针，如果不存在，就扩容对象池
     *  然后，检查fd对象是否存在相同的事件
     *  接着，创建epoll事件对象，并注册事件
     *  最后，更新fd对象的事件处理器
     */
    FdContext* fd_ctx;
    MutexType::ReadLock lock(&m_mutex);
    if (m_fd_context_list.size() > fd)
    {
        fd_ctx = m_fd_context_list[fd].get();
        lock.unlock();
    }
    else
    {
        lock.unlock();
        MutexType::WriteLock lock2(&m_mutex);
        contextListResize(m_fd_context_list.size() * 1.5);
        fd_ctx = m_fd_context_list[fd].get();
    }

    FdContext::MutexType::Lock lock3(&fd_ctx->m_mutex);
    // 检查要监听的事件是否已经存在
    int is_same = fd_ctx->m_events & event;
    if (is_same)
    {
        LOG_FMT_ERROR(g_logger, "IOManager::addEvent | exist same event | fd=%d, event=%d, fd_ctx.event=%d", (int)fd, event, fd_ctx->m_events);
        ASSERT(false);
    }

    /**
     * 如果这个fd context的 m_events 是空的，说明这个fd还没在epoll上注册
     * 使用 EPOLL_CTL_ADD 注册新事件
     * 否则，使用 EPOLL_CTL_MOD 更改fd监听的事件
     */
    int op = fd_ctx->m_events == EventType::NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    epoll_event ep_event{};
    ep_event.events   = EPOLLET | fd_ctx->m_events | event;
    ep_event.data.ptr = fd_ctx;
    // 给fd注册事件监听
    int ep_ctl_res = ::epoll_ctl(m_epoll_fd, op, fd, &ep_event);
    if (ep_ctl_res)
    {
        LOG_FMT_ERROR(g_logger, "IOManager::addEvent |epoll_ctl failed, epfd=%d", m_epoll_fd);
        return -1;
    }

    LOG_FMT_DEBUG(g_logger, "epoll_ctl %s register event | %ul : %s",
                  op == EPOLL_CTL_ADD ? "ADD" : "MOD",
                  ep_event.events,
                  strerror(errno));

    ++m_pending_event_count;

    fd_ctx->m_events                              = static_cast<EventType>(fd_ctx->m_events | event);
    IOManager::FdContext::EventContext& event_ctx = fd_ctx->getEventContext(event);
    // 确保没有给这个 fd 重复添加事件监听
    ASSERT(event_ctx.m_scheduler == nullptr &&
           !event_ctx.m_fiber &&
           !event_ctx.m_callback);
    event_ctx.m_scheduler = Scheduler::GetThis();
    if (callback)
    {
        event_ctx.m_callback.swap(callback);
    }
    else
    {
        // 如果 callback 是 nullptr 时，将当前上下转换为协程，并作为时间回调使用
        event_ctx.m_fiber = Fiber::GetThis();
    }

    return 0;
}

bool IOManager::removeEvent(int fd, EventType event)
{
    FdContext* fd_ctx = nullptr;
    {
        MutexType::ReadLock lock(&m_mutex);
        if (m_fd_context_list.size() <= fd)
        {
            return false;
        }
        fd_ctx = m_fd_context_list[fd].get();
    }

    FdContext::MutexType::Lock lock2(&fd_ctx->m_mutex);
    if (!(fd_ctx->m_events & event))
    {
        LOG_FMT_INFO(g_logger, "event context not exist | eventType=%d", event);
        return false;
    }

    auto new_events = static_cast<EventType>(fd_ctx->m_events & ~event);
    int op          = new_events == EventType::NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    epoll_event ep_event{};
    ep_event.data.ptr = fd_ctx;
    ep_event.events   = EPOLLET | new_events;
    int ep_ctl_res    = ::epoll_ctl(m_epoll_fd, op, fd, &ep_event);
    ASSERT_M(ep_ctl_res == 0, "IOManager::removeEvent | epoll ctl failed | epfd=" + std::to_string(m_epoll_fd));

    fd_ctx->m_events = new_events;
    auto& event_ctx  = fd_ctx->getEventContext(event);
    fd_ctx->resetEventContext(event_ctx);
    --m_pending_event_count;

    return true;
}

bool IOManager::cancelEvent(int fd, EventType event)
{
    FdContext* fd_ctx = nullptr;
    {
        MutexType::ReadLock lock(&m_mutex);
        if (m_fd_context_list.size() <= fd)
        {
            false;
        }
        fd_ctx = m_fd_context_list[fd].get();
    }

    FdContext::MutexType::Lock lock(&fd_ctx->m_mutex);
    if (!(fd_ctx->m_events & event))
    {
        LOG_FMT_INFO(g_logger, "event not exist | eventType=%d", event);
        return false;
    }

    auto new_event = static_cast<EventType>(fd_ctx->m_events & ~event);
    int op         = new_event == EventType::NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;

    epoll_event ep_event{};
    ep_event.data.ptr = fd_ctx;
    ep_event.events   = EPOLLET | new_event;
    int ep_ctl_res    = epoll_ctl(m_epoll_fd, op, fd, &ep_event);
    ASSERT_M(ep_ctl_res == 0, "IOManager::cancelEvent | epoll_ctl failed | epfd=" + std::to_string(m_epoll_fd));

    fd_ctx->m_events = new_event;

    auto& event_ctx  = fd_ctx->getEventContext(event);
    fd_ctx->triggerEventContext(event);
    --m_pending_event_count;

    return true;
}

bool IOManager::cancelAllEvent(int fd)
{
    FdContext* fd_ctx = nullptr;
    {
        MutexType::ReadLock lock(&m_mutex);
        if (m_fd_context_list.size() <= fd)
        {
            return false;
        }
        fd_ctx = m_fd_context_list[fd].get();
    }
    FdContext::MutexType::Lock lock(&fd_ctx->m_mutex);

    if (!fd_ctx->m_events)
    {
        return false;
    }

    // 移除监听
    int ep_ctl_res = ::epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    ASSERT_M(ep_ctl_res == 0, "IOManager::cancelAllEvent | epoll_ctl failed | epfd=" + std::to_string(m_epoll_fd));

    if (fd_ctx->m_events & EventType::READ)
    {
        fd_ctx->triggerEventContext(EventType::READ);
        --m_pending_event_count;
    }

    if (fd_ctx->m_events & EventType::WRITE)
    {
        fd_ctx->triggerEventContext(EventType::WRITE);
        --m_pending_event_count;
    }

    fd_ctx->m_events = EventType::NONE;

    return true;
}

IOManager* IOManager::GetThis()
{
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle()
{
    if (hasIdleThreads())
    {
        return;
    }
    int rt = ::write(m_tickle_fds[1], "T", 1);
    ASSERT(rt == 1);
}

bool IOManager::isStop()
{
    return m_pending_event_count == 0 &&
           !hasTimer() &&
           Scheduler::isStop();
}

bool IOManager::isStop(uint64_t& next_timeout)
{
    next_timeout = getNextTimer();
    return next_timeout == ~0ull &&
           m_pending_event_count == 0 &&
           Scheduler::isStop();
}

void IOManager::idle()
{
    epoll_event* ep_events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(ep_events, [](epoll_event* events)
                                               { delete[] events; });
    while (true)
    {
        uint64_t next_timeout = 0;
        if (isStop(next_timeout))
        {
            LOG_FMT_DEBUG(g_logger, "IOManager::idle is stop and exist | %s", m_name.c_str());
            break;
        }
        static const int EPOLL_TIME_OUT = 1000;
        if (next_timeout == ~0ull || EPOLL_TIME_OUT < next_timeout)
        {
            next_timeout = EPOLL_TIME_OUT;
        }

        int rt = 0;
        do
        {
            // 阻塞等待 epoll_wait 返回结果，若超时中断，下镒继续重试
            rt = ::epoll_wait(m_epoll_fd, ep_events, 64, next_timeout);
            if (rt < 0 && errno == EINTR)
            {
                // continue
            }
            else
            {
                break;
            }
        } while (true);

        std::vector<std::function<void()>> fns;
        listExpiredTimers(fns);
        if (!fns.empty())
        {
            // LOG_FMT_DEBUG(g_logger, "fns size=%d................", (int)fns.size());
            schedule(fns.begin(), fns.end(), -1);
            fns.clear();
        }

        // 遍历被 ep_events 处理被触发的 fd
        for (int i = 0; i < rt; i++)
        {
            epoll_event& event = ep_events[i];
            // 接收来自主线程的消息
            if (event.data.fd == m_tickle_fds[0])
            {
                char dummy;
                // 将来自主线程的消息读取干净
                while (true)
                {
                    int status = ::read(event.data.fd, &dummy, 1);
                    if (status == 0 || status == -1)
                    {
                        break;
                    }
                }
                continue;
            }

            // 处理非主线程的消息
            auto fd_ctx = static_cast<FdContext*>(event.data.ptr);
            FdContext::MutexType::Lock lock(&fd_ctx->m_mutex);
            // 如果该事件 fd 出现错误或已经失效（中断）
            if (event.events & (EPOLLERR | EPOLLHUP))
            {
                // 加入读或写事件，唤醒 fd 的事件监听
                event.events |= EPOLLIN | EPOLLOUT;
            }

            uint32_t real_events = EventType::NONE;
            if (event.events & EPOLLIN)
            {
                real_events |= EventType::READ;
            }
            if (event.events & EPOLLOUT)
            {
                real_events |= EventType::WRITE;
            }

            // fd 中指定的事件已经被触发并处理，不做操作了
            if ((fd_ctx->m_events & real_events) == EventType::NONE)
            {
                continue;
            }

            // 从 epoll 移除这个 fd 的对应被触发的事件
            int left_events = fd_ctx->m_events & ~real_events;
            int op          = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            int new_events  = EPOLLET | left_events;
            epoll_event ep_event{};
            ep_event.data.ptr = fd_ctx;
            ep_event.events   = EPOLLET | new_events;
            int rt2           = ::epoll_ctl(m_epoll_fd, op, fd_ctx->m_fd, &ep_event);
            // epoll_ctl 执行失败，打印日志，不做操作了
            if (rt2 == -1)
            {
                LOG_FMT_ERROR(g_logger, "IOManager::idle epoll_ctl failed | epfd=%d, rt2=%d",
                              fd_ctx->m_fd, rt2);
                continue;
            }

            // 触发 fd 对应事件的处理器
            if (real_events & EventType::READ)
            {
                fd_ctx->triggerEventContext(EventType::READ);
                --m_pending_event_count;
            }
            if (real_events & EventType::WRITE)
            {
                fd_ctx->triggerEventContext(EventType::WRITE);
                --m_pending_event_count;
            }
        }

        // 让出当前线程的执行权，给调度器执行排队等待的协程
        Fiber::ptr cur_fiber = Fiber::GetThis();
        Fiber* raw_ptr       = cur_fiber.get();
        cur_fiber.reset();
        raw_ptr->swap_out();
    }
}

/**
 * ============================================================================
 * IOManager::FdContext 类的实现
 * ============================================================================
 */

IOManager::FdContext::EventContext& IOManager::FdContext::getEventContext(EventType event)
{
    switch (event)
    {
        case EventType::READ:
            return m_read;
        case EventType::WRITE:
            return m_write;

        default:
            ASSERT_M(false, "event type not found | " + event);
    }
}

bool IOManager::FdContext::resetEventContext(EventContext& event_ctx)
{
    event_ctx.m_scheduler = nullptr;
    event_ctx.m_callback  = nullptr;
    event_ctx.m_fiber.reset();
    return true;
}

bool IOManager::FdContext::triggerEventContext(EventType event)
{
    ASSERT(m_events & event);

    m_events                = static_cast<EventType>(m_events & ~event);

    EventContext& event_ctx = getEventContext(event);
    ASSERT(event_ctx.m_scheduler);

    if (event_ctx.m_callback)
    {
        event_ctx.m_scheduler->schedule(std::move(event_ctx.m_callback));
    }
    else
    {
        event_ctx.m_scheduler->schedule(std::move(event_ctx.m_fiber));
    }
    event_ctx.m_scheduler = nullptr;
    return true;
}

} // namespace trycle