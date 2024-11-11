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
    : m_name(name)
{
    if (use_caller)
    {
        --thread_size;
        m_thread_count = thread_size;

        ASSERT(GetThis() == nullptr);
        set_to_this();

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
    m_auto_stop = true;
    if (m_root_fiber)
    {
        if (m_thread_count == 0 && (m_root_fiber->get_state() == Fiber::INIT ||
                                    m_root_fiber->get_state() == Fiber::TERM))
        {
            LOG_FMT_INFO(g_logger, "schduler stop | name=%s", m_name.c_str());
            m_stopping = true;
            if (stopping())
            {
                return;
            }
        }
    }

    bool exit_on_this_fiber = false;
    if (m_root_thread_id == -1)
    {
        ASSERT(GetThis() == this);
    }
    else
    {
        ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for (int i = 0; i < m_thread_count; i++)
    {
        tickle();
    }

    if (stopping())
    {
        tickle();
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
}

void Scheduler::run()
{
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
                break;
            }
        }

        if (tickle_me)
        {
            tickle();
        }

        if (ft.fiber && ft.fiber->get_state() != Fiber::TERM)
        {
            ++m_active_thread_count;
            ft.fiber->swap_in();
            --m_active_thread_count;

            if (ft.fiber->get_state() == Fiber::READY)
            {
                schedule(ft.fiber);
            }
            else if (ft.fiber->get_state() != Fiber::EXCEPT &&
                     ft.fiber->get_state() != Fiber::TERM)
            {
                ft.fiber->set_state(Fiber::HOLD);
            }
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

            ++m_active_thread_count;
            cb_fiber->swap_in();
            --m_active_thread_count;

            if (cb_fiber->get_state() == Fiber::READY)
            {
                schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if (cb_fiber->get_state() == Fiber::EXCEPT || cb_fiber->get_state() == Fiber::TERM)
            {
                cb_fiber->reset(nullptr);
            }
            else
            {
                ft.fiber->set_state(Fiber::HOLD);
                cb_fiber.reset();
            }
        }
        else
        {
            if (idle_fiber->get_state() == Fiber::TERM)
            {
                break;
            }

            ++m_idle_thread_count;
            idle_fiber->swap_in();
            if (cb_fiber->get_state() != Fiber::EXCEPT || cb_fiber->get_state() != Fiber::TERM)
            {
                idle_fiber->set_state(Fiber::HOLD);
            }
            --m_idle_thread_count;
        }
    }
}

bool Scheduler::stopping()
{
    // TODO
    return true;
}

void Scheduler::idle()
{
}

} // namespace trycle
