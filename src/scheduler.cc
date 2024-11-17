#include "scheduler.h"
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "thread.h"

namespace trycle
{

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber         = nullptr;

static auto g_logger                       = GET_LOGGER("system");

Scheduler::Scheduler(int thread_size, bool use_caller, const std::string& name)
    : m_name(name),
      m_thread_count(thread_size)
{
    if (use_caller)
    {
        // 实例化此类的线程作为master fiber
        Fiber::GetThis();
        // 线程池需要的线程减1
        m_thread_count -= 1;
        // 确保该线程只有一个高度器
        ASSERT(GetThis() == nullptr);
        set_to_this();
        // 因为Scheduler::run()是实例化方法，需要用std::bind绑定调用者
        m_root_fiber.reset(new Fiber(std::bind(&Scheduler::run, this)));

        t_fiber          = m_root_fiber.get();
        m_root_thread_id = GetThreadId();
        m_thread_ids.insert(m_root_thread_id);
    }
    else
    {
        m_root_thread_id = -1;
    }
}

Scheduler::~Scheduler()
{
    ASSERT(m_stopping);
    if (GetThis() == this)
    {
        t_scheduler = nullptr;
    }
}

void Scheduler::start()
{
    LOG_DEBUG(g_logger, "Scheduler::start");
    MutexType::Lock lock(&m_mutex);
    if (!m_stopping)
    {
        return;
    }
    m_stopping = false;
    ASSERT(m_threads.empty());
    m_threads.resize(m_thread_count);
    for (int i = 0; i < m_thread_count; i++)
    {
        m_threads[i] = Thread::ptr(new Thread(m_name + std::to_string(i), std::bind(&Scheduler::run, this)));
        m_thread_ids.insert(m_threads[i]->get_id());
    }
}

void Scheduler::stop()
{
    LOG_DEBUG(g_logger, "Scheduler::stop");
    m_auto_stop = true;
    if (m_root_fiber)
    {
        if (m_thread_count == 0 && (m_root_fiber->isFinish() || m_root_fiber->get_state() == Fiber::INIT))
        {
            LOG_FMT_INFO(g_logger, "schduler stop | name=%s", m_name.c_str());
            m_stopping = true;
            if (isStop())
            {
                return;
            }
        }
    }

    m_stopping = true;
    for (int i = 0; i < m_thread_count; i++)
    {
        tickle();
    }

    if (m_root_fiber)
    {
        tickle();
        if (!isStop())
        {
            m_root_fiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(&m_mutex);
        thrs.swap(m_threads);
    }

    { // join所有子线程
        for (const auto& item : thrs)
        {
            item->join();
        }
        m_threads.clear();
    }
}

Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}
void Scheduler::set_to_this()
{
    t_scheduler = this;
}

Fiber* Scheduler::GetMainFiber()
{
    return t_fiber;
}

void Scheduler::tickle()
{
    LOG_DEBUG(g_logger, "Scheduler::tickle");
}

void Scheduler::run()
{
    LOG_DEBUG(g_logger, "Scheduler::run");
    set_to_this();
    if (GetThreadId() != m_root_thread_id)
    {
        t_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;
    FiberAndThread ft;

    while (true)
    {
        ft.reset();

        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(&m_mutex);
            for (auto it = m_fibers.begin(); it != m_fibers.end(); ++it)
            {
                if (it->thread != -1 && it->thread != m_root_thread_id)
                {
                    tickle_me = true;
                    continue;
                }
                ASSERT(it->cb || it->fiber);

                if (it->fiber && it->fiber->get_state() == Fiber::EXEC)
                {
                    continue;
                }

                ft = *it;
                m_fibers.erase(it);
                ++m_active_thread_count;
                is_active = true;
                break;
            }
        }

        if (tickle_me)
        {
            tickle();
        }

        if (ft.fiber && !ft.fiber->isFinish())
        {
            ft.fiber->swap_in();
            --m_active_thread_count;

            if (ft.fiber->get_state() == Fiber::READY)
            {
                schedule(ft.fiber);
            }
            else if (!ft.fiber->isFinish())
            {
                ft.fiber->set_state(Fiber::HOLD);
            }
            ft.reset();
        }
        else if (ft.cb)
        {
            if (cb_fiber)
            {
                cb_fiber->reset(ft.cb);
            }
            else
            {
                cb_fiber.reset(new Fiber(ft.cb));
            }

            ft.reset();
            cb_fiber->swap_in();
            --m_active_thread_count;

            if (cb_fiber->get_state() == Fiber::READY)
            {
                schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if (cb_fiber->isFinish())
            {
                cb_fiber->reset(nullptr);
            }
            else
            {
                cb_fiber->set_state(Fiber::HOLD);
                cb_fiber.reset();
            }
        }
        else
        {
            if (is_active)
            {
                --m_active_thread_count;
                continue;
            }

            if (idle_fiber->isFinish())
            {
                LOG_INFO(g_logger, "idle fiber finish");
                break;
            }

            ++m_idle_thread_count;
            idle_fiber->swap_in();
            --m_idle_thread_count;
            if (!idle_fiber->isFinish())
            {
                idle_fiber->set_state(Fiber::HOLD);
            }
        }
    }
}

bool Scheduler::isStop()
{
    // LOG_DEBUG(g_logger, "Scheduler::isStop()");
    MutexType::Lock lock(&m_mutex);
    return m_auto_stop && m_stopping && m_active_thread_count == 0 && m_fibers.empty();
}

void Scheduler::idle()
{
    LOG_DEBUG(g_logger, "Scheduler::idle");
    while (!isStop())
    {
        Fiber::YieldToHold();
    }
}

} // namespace trycle
