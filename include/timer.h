#ifndef TRY_TIMER_H
#define TRY_TIMER_H

#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "thread.h"

namespace trycle
{
class TimerManager;
class Timer : public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;

public:
    typedef std::shared_ptr<Timer> ptr;
    typedef std::function<void()> FuncCb;
    // std::enable_shared_from_this
public:
    Timer(uint64_t ms, bool cyclic, FuncCb fn, TimerManager* tm);
    Timer(uint64_t next);

    bool reset(uint64_t ms, bool from_now);
    bool refresh();
    bool cancel();

protected:
    struct TimerComparator
    {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };

private:
    bool m_cyclic   = false;
    uint64_t m_ms   = 0;
    uint64_t m_next = 0;
    FuncCb m_fn;
    TimerManager* m_tm;
};

class TimerManager
{
    friend class Timer;

public:
    typedef RWMutex MutexType;

public:
    TimerManager();
    ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> fn, bool cyclic);
    bool addTimer(Timer::ptr val, MutexType::WriteLock& lock);

    void addConditionTimer(uint64_t ms, std::function<void()> fn, std::weak_ptr<void> weak_cond, bool cyclic);
    uint64_t getNextTimer();
    bool hasTimer();

    virtual void onTimerInsertedAtFirst() = 0;

    void listExpiredTimers(std::vector<std::function<void()>>& fns);
    bool detectTimeRollover(uint64_t now_ms);

private:
    MutexType m_mutex;
    std::set<Timer::ptr, Timer::TimerComparator> m_timers;
    uint64_t m_previous_time = 0;
};

} // namespace trycle

#endif // TRY_TIMER_H