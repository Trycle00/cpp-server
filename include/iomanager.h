#ifndef TRY_IO_MANAGER_H
#define TRY_IO_MANAGER_H

#include <functional>

#include "scheduler.h"
#include "timer.h"

namespace trycle
{

class IOManager : public Scheduler, public TimerManager
{

public:
    enum EventType
    {
        NONE  = 0x0,
        READ  = 0x1, // EPOLLIN
        WRITE = 0x4  // EPOLLOUT
    };

    struct FdContext
    {
        typedef std::shared_ptr<FdContext> ptr;
        typedef Mutex MutexType;

        struct EventContext
        {
            Scheduler* m_scheduler;           // 指定处理该事件的调度器
            Fiber::ptr m_fiber;               // 要执行的协程
            std::function<void()> m_callback; // 要执行的函数，fiber与callback，只需要其一
        };

        EventContext& getEventContext(EventType event);
        bool resetEventContext(EventContext& event_ctx);
        bool triggerEventContext(EventType event);

        MutexType m_mutex;
        EventContext m_read;  // 处理读事件
        EventContext m_write; // 处理写事件
        int m_fd{};           // 要监听的文件描述符
        EventType m_events = EventType::NONE;
    };

public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex MutexType;

    IOManager(size_t thread_size = 1, bool use_caller = false, const std::string& name = "");
    ~IOManager();

    bool addEvent(int fd, EventType event, std::function<void()> callback = nullptr);
    bool removeEvent(int fd, EventType event);
    bool cancelEvent(int fd, EventType event);
    bool cancelAllEvent(int fd);

public:
    static IOManager* GetThis();

protected:
    void tickle() override;
    bool isStop() override;
    bool isStop(uint64_t& next_timeout);
    void idle() override;

    void contextListResize(int size);

    void onTimerInsertedAtFirst();

private:
    MutexType m_mutex;
    int m_epoll_fd = 0;                            // epoll文件标识符
    int m_tickle_fds[2]{};                         // 主线程给子线程发送消息的管道
    std::atomic_size_t m_pending_event_count{};    // 等待执行的事件数量
    std::vector<FdContext::ptr> m_fd_context_list; // FdContext的对象池，下标对应fd id
};

} // namespace trycle

#endif // TRY_IO_MANAGER_H