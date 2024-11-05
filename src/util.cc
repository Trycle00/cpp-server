#include "util.h"

#include <thread>
#if _WIN32
#else
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace trycle
{

uint64_t GetThreadId()
{
#if _WIN32
    std::thread::id thread_id = std::this_thread::get_id();
    uint32_t id               = *(uint64_t*)&thread_id;
    return id;
#elif defined(__linux__)
    return syscall(SYS_gettid);
#elif defined(__FreeBSD__)
    long tid;
    thr_self(&tid);
    return (int)tid;
#elif defined(__NetBSD__)
    return _lwp_self();
#elif defined(__OpenBSD__)
    return getthrid();
#else
    return getpid();
#endif
    // uint32_t id = *static_cast<uint32_t*>(static_cast<void*>(&thread_id));
}

uint64_t GetFiberId()
{
    return 0;
}

} // namespace trycle