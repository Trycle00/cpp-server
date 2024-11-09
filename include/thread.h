#ifndef TRY_THREAD_H
#define TRY_THREAD_H

#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <thread>

namespace trycle
{

// 信号量
class Semaphore
{
public:
    Semaphore(const int count = 0);

    void wait();
    void notify();

private:
    sem_t m_semaphore;
};

// 锁--实现类
template <typename T>
class ScopedLockImpl
{
public:
    ScopedLockImpl(T* mutex)
        : m_mutex(mutex)
    {
        m_mutex->lock();
        m_locked = true;
    }

    ~ScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if (!m_locked)
        {
            m_mutex->lock();
        }
    }
    void unlock()
    {
        if (m_locked)
        {
            m_mutex->unlock();
        }
    }

private:
    T* m_mutex;
    bool m_locked;
};

// 读锁--实现类
template <typename T>
class ReadScopedLockImpl
{
public:
    ReadScopedLockImpl(T* mutex)
        : m_mutex(mutex)
    {
        m_mutex->rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if (!m_locked)
        {
            m_mutex->rdlock();
        }
    }
    void unlock()
    {
        if (m_locked)
        {
            m_mutex->unlock();
        }
    }

private:
    T* m_mutex;
    bool m_locked;
};

// 写锁--实现类
template <typename T>
class WriteScopedLockImpl
{
public:
    WriteScopedLockImpl(T* mutex)
        : m_mutex(mutex)
    {
        m_mutex->wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if (!m_locked)
        {
            m_mutex->wrlock();
        }
    }

    void unlock()
    {
        if (m_locked)
        {
            m_mutex->unlock();
        }
    }

private:
    T* m_mutex;
    bool m_locked;
};

// 互斥量--锁
class Mutex
{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex()
    {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;
};

class NullMutex
{
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

// 互斥量--读写锁
class RWMutex
{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef ReadScopedLockImpl<RWMutex> WriteLock;

    RWMutex()
    {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);
    }

    void unlock()
    {
        pthread_rwlock_unlock(&m_lock);
    }

    void rdlock()
    {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock()
    {
        pthread_rwlock_wrlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};

class NullRWMutex
{

public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;

    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

class SpinMutex
{
public:
    typedef ScopedLockImpl<SpinMutex> Lock;

    SpinMutex()
    {
        pthread_spin_init(&m_lock, 0);
    }
    ~SpinMutex()
    {
        pthread_spin_destroy(&m_lock);
    }

    void lock()
    {
        pthread_spin_lock(&m_lock);
    }
    void unlock()
    {
        pthread_spin_unlock(&m_lock);
    }

private:
    pthread_spinlock_t m_lock;
};

// class CASMutex
// {
//     static const int UNLOCK = 0;
//     static const int LOCKED = 1;

// public:
//     typedef ScopedLockImpl<CASMutex> Lock;

//     CASMutex()
//     {
//         m_value.store(LOCKED);
//     }

//     ~CASMutex()
//     {
//         unlock();
//     }

//     void lock()
//     {
//         while (true)
//         {

//             int expected = UNLOCK;
//             if (m_value.compare_exchange_strong(expected, LOCKED))
//             {
//                 break;
//             }
//         }
//     }

//     void unlock()
//     {
//         m_value.store(UNLOCK);
//     }

// private:
//     std::atomic<int> m_value;
// };

// 线程类
class Thread
{
public:
    typedef std::shared_ptr<Thread> ptr;
    typedef std::function<void()> ThreadCb;

    Thread(const std::string&, const ThreadCb&);
    ~Thread();

    pid_t get_id() { return m_id; }
    void set_id(const pid_t id) { m_id = id; }
    std::string get_name() { return m_name; }
    void set_name(const std::string& name) { m_name = name; }

    void join();

    static void* run(void* arg);

    static Thread* get_current_thread();
    static const pid_t get_current_id();
    static const std::string get_current_name();

private:
    Thread() = delete;

private:
    pid_t m_id          = -1;
    pthread_t m_pthread = 0;
    ThreadCb m_cb;
    std::string m_name;
    Semaphore m_semaphore;
};

} // namespace trycle

#endif // TRY_THREAD_H