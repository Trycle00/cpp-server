#include "thread.h"
#include "log.h"

namespace trycle
{

Logger::ptr g_logger = GET_LOGGER("system");

// static thread_local pid_t g_pid = 0;
static thread_local Thread* t_thread          = nullptr;
static thread_local std::string t_thread_name = "<UNKNOWN>";

Semaphore::Semaphore(const int count)
{
    int result = sem_init(&m_semaphore, 0, count);
    if (result)
    {
        throw std::logic_error("sem_init() error");
    }
}

void Semaphore::wait()
{
    int result = sem_wait(&m_semaphore);
    if (result)
    {
        throw std::logic_error("sem_wait() error");
    }
}

void Semaphore::notify()
{
    int result = sem_post(&m_semaphore);
    if (result)
    {
        throw std::logic_error("sem_post() error");
    }
}

Thread* Thread::get_current_thread()
{
    if (t_thread)
    {
        return t_thread;
    }
    return nullptr;
}

const pid_t Thread::get_current_id()
{
    if (t_thread)
    {
        return t_thread->get_id();
    }
    return 0;
}

const std::string Thread::get_current_name()
{
    return t_thread_name;
}

Thread::Thread(const std::string& name, const ThreadCb& cb)
    : m_cb(cb),
      m_name(name)
{
    int result = pthread_create(&m_pthread, nullptr, &Thread::run, this);
    if (result)
    {
        LOG_FMT_ERROR(g_logger, "pthread_create() error | name=%s, result=%d", m_name.c_str(), result);
        throw std::exception();
    }

    m_semaphore.wait();
}

Thread::~Thread()
{
    if (m_pthread)
    {
        pthread_detach(m_pthread);
    }
}

void Thread::join()
{
    if (m_pthread)
    {
        int result = pthread_join(m_pthread, nullptr);
        if (result)
        {
            LOG_FMT_ERROR(g_logger, "pthread_join() error | name=%s, result=%d", m_name.c_str(), result);
            throw std::exception();
        }
        m_pthread = 0;
    }
}

void* Thread::run(void* arg)
{
    Thread* thread = (Thread*)arg;
    thread->m_id   = trycle::GetThreadId();

    t_thread       = thread;
    t_thread_name  = thread->m_name;

    pthread_setname_np(thread->m_pthread, thread->m_name.substr(0, 15).c_str());

    thread->m_semaphore.notify();

    ThreadCb cb;
    thread->m_cb.swap(cb);

    cb();

    return 0;
}

} // namespace trycle