#ifndef TRY_SCHEDULER_H
#define TRY_SCHEDULER_H

#include <functional>
#include <list>
#include <memory>
#include <set>
#include <vector>

#include "fiber.h"
#include "thread.h"

namespace trycle
{

// static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_fiber_ptr;

class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler() = delete;
    Scheduler(int thread_size, bool use_caller, const std::string& name);
    virtual ~Scheduler();

    const std::string& get_name() const { return m_name; }

    void start();
    void stop();

    template <typename FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(&m_mutex);
            need_tickle = schedule_without_lock(fc, thread);
        }
        if (need_tickle)
        {
            tickle();
        }
    }

    template <typename InputIterator>
    void schedule(InputIterator begin, InputIterator end, int thread)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(&m_mutex);
            while (begin++ != end)
            {
                auto ft     = &*begin;
                need_tickle = schedule_without_lock(ft, thread) || need_tickle;
            }
        }
        if (need_tickle)
        {
            tickle();
        }
    }

public:
    static Scheduler* GetThis();
    // static void SetThis(Scheduler* s);
    static Fiber* GetMainFiber();

protected:
    void set_to_this();
    void tickle();
    void run();
    bool stopping();
    void idle();

private:
    template <typename FiberOrCb>
    bool schedule_without_lock(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = !m_fibers.empty();

        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb)
        {
            m_fibers.push_back(ft);
        }

        return need_tickle;
    }

    struct FiberAndThread
    {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr ptr, const int t)
            : fiber(ptr),
              thread(t) {}

        FiberAndThread(Fiber::ptr* ptr, const int t)
            : thread(t)
        {
            fiber.swap(*ptr);
        }

        FiberAndThread(std::function<void()> cb, const int t)
            : cb(cb),
              thread(t) {}

        FiberAndThread(std::function<void()>* cb, const int t)
            : thread(t)
        {
            cb->swap(*cb);
        }

        FiberAndThread()
            : thread(-1) {}

        void reset()
        {
            fiber  = nullptr;
            cb     = nullptr;
            thread = -1;
        }
    };

protected:
    std::set<int> m_thread_ids;
    int m_thread_count;
    int m_active_thread_count;
    int m_idle_thread_count;
    bool m_stopping;
    bool m_auto_stop;
    int m_root_thread_id;

private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_root_fiber;
    std::string m_name;
};

} // namespace trycle

#endif // TRY_SCHEDULER_H