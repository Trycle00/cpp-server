#include "timer.h"
#include "util.h"

namespace trycle
{
/**
 * ============================================================================
 * Timer 类的实现
 * ============================================================================
 */
Timer::Timer(uint64_t ms, bool cyclic, FuncCb fn, TimerManager* tm)
    : m_ms(ms),
      m_cyclic(cyclic),
      m_fn(fn),
      m_tm(tm)
{
    m_next = GetCurrentMs() + ms;
}

Timer::Timer(uint64_t next)
    : m_next(next)
{
}

bool Timer::reset(uint64_t ms, bool from_now)
{
    TimerManager::MutexType::WriteLock lock(&m_tm->m_mutex);

    if (ms == m_ms && !from_now)
    {
        return false;
    }
    if (!m_fn)
    {
        return false;
    }

    auto it = m_tm->m_timers.find(shared_from_this());
    if (it == m_tm->m_timers.end())
    {
        return false;
    }

    m_tm->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now)
    {
        start = GetCurrentMs();
    }
    else
    {
        start = m_next - m_ms;
    }
    m_ms   = ms;
    m_next = start + m_ms;
    // m_tm->m_timers.insert(shared_from_this());
    m_tm->addTimer(shared_from_this(), lock);

    return true;
}

bool Timer::refresh()
{
    TimerManager::MutexType::WriteLock lock(&m_tm->m_mutex);
    if (!m_fn)
    {
        return false;
    }

    auto it = m_tm->m_timers.find(shared_from_this());
    if (it == m_tm->m_timers.end())
    {
        return false;
    }
    m_next = GetCurrentMs() + m_ms;
    m_tm->m_timers.insert(shared_from_this());

    return true;
}

bool Timer::cancel()
{
    TimerManager::MutexType::WriteLock lock(&m_tm->m_mutex);
    if (!m_fn)
    {
        return false;
    }

    m_fn = nullptr;
    m_tm->m_timers.erase(shared_from_this());

    return true;
}

/**
 * ============================================================================
 * TimerComparator 类的实现
 * ============================================================================
 */
bool Timer::TimerComparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
{
    if (!lhs && !rhs)
    {
        return false;
    }
    if (!lhs)
    {
        return true;
    }
    if (!rhs)
    {
        return false;
    }
    if (lhs->m_next < rhs->m_next)
    {
        return true;
    }
    if (lhs->m_next > rhs->m_next)
    {
        return false;
    }
    return lhs.get() < rhs.get();
}

/**
 * ============================================================================
 * TimerManager 类的实现
 * ============================================================================
 */
TimerManager::TimerManager()
{
    m_previous_time = GetCurrentMs();
}

TimerManager::~TimerManager()
{
}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> fn, bool cyclic)
{
    Timer::ptr timer(new Timer(ms, cyclic, fn, this));
    MutexType::WriteLock lock(&m_mutex);
    m_timers.insert(timer);
    addTimer(timer, lock);
    return timer;
}

bool TimerManager::addTimer(Timer::ptr val, MutexType::WriteLock& lock)
{
    auto it       = m_timers.insert(val).first;
    bool at_front = it == m_timers.begin();
    if (at_front)
    {
        onTimerInsertedAtFirst();
    }
    return true;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> fn)
{
    auto cond = weak_cond.lock();
    if (cond)
    {
        fn();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> fn,
                                           std::weak_ptr<void> weak_cond, bool cyclic)
{
    Timer::ptr timer(new Timer(ms, cyclic, fn, this));
    {
        MutexType::WriteLock lock(&m_mutex);
        addTimer(ms, std::bind(&OnTimer, weak_cond, fn), cyclic);
    }
    return timer;
}

void TimerManager::listExpiredTimers(std::vector<std::function<void()>>& fns)
{
    std::vector<Timer::ptr> expireds;
    {
        MutexType::ReadLock lock(&m_mutex);
        if (m_timers.empty())
        {
            return;
        }
    }

    auto now_ms = GetCurrentMs();

    MutexType::WriteLock lock(&m_mutex);

    // 检查调整系统时间是否被修改
    bool rolllover = detectTimeRollover(now_ms);
    // 如果系统时间正常，才需要检查首个任务是否超时，
    if (!rolllover && (*m_timers.begin())->m_next > now_ms)
    {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    // 如果系统时间被回拨，直接认为所有任务都已经超时，需要执行
    // 如果没有，则通过 lower_bound 找出第一个大于或等于 now_timer 定时器的迭代器
    auto it = rolllover ? m_timers.end() : m_timers.lower_bound(now_timer);
    // 如果不是结束位，并且，仍然有相同的时间，则定位到下一个定时器
    while (it != m_timers.end() && it->get()->m_next == now_ms)
    {
        ++it;
    }

    // 取出所有超时的代码器，取值范围[n,m)
    expireds.insert(expireds.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    fns.reserve(expireds.size());

    for (auto& it : expireds)
    {
        fns.push_back(it->m_fn);
        // 处理周期循环的定时器
        if (it->m_cyclic)
        {
            // 若是，定时器指定下次执行时间后，添加到定时器队列
            it->m_next = now_ms + it->m_ms;
            m_timers.insert(it);
        }
        else
        {
            it->m_fn = nullptr;
        }
    }
}

uint64_t TimerManager::getNextTimer()
{
    MutexType::ReadLock lock(&m_mutex);
    if (m_timers.empty())
    {
        // 返回最大值，表示没有timer
        return ~0ull;
    }
    auto now_ms = GetCurrentMs();
    auto timer  = *m_timers.begin();
    if (timer->m_next <= now_ms)
    {
        // 有任务超时，返回0表示立即执行
        return 0;
    }

    // 返回还需要等待的时间
    return timer->m_next - now_ms;
}

bool TimerManager::hasTimer()
{
    MutexType::ReadLock lock(&m_mutex);
    return !m_timers.empty();
}

bool TimerManager::detectTimeRollover(uint64_t now_ms)
{
    static uint64_t ROLLOVER_TIME = 60 * 60 * 1000;
    bool rollover                 = false;
    // 系统时间被回拨，超过1 ROLLOVER_TIME 小时
    if (m_previous_time > now_ms &&
        m_previous_time - ROLLOVER_TIME > now_ms)
    {
        rollover = true;
    }
    m_previous_time = now_ms;
    return rollover;
}

} // namespace trycle
