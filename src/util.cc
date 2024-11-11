#include "util.h"

#include <execinfo.h>
#include <sstream>
#include <thread>
#if _WIN32
#else
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include "fiber.h"
#include "log.h"

namespace trycle
{

static Logger::ptr g_logger = GET_LOGGER("system");

uint32_t GetThreadId()
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

uint32_t GetFiberId()
{
    return trycle::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& vec, int size, int skip)
{
    void** array   = (void**)malloc(sizeof(void*) * size);
    size_t nptrs   = ::backtrace(array, size);

    char** strings = ::backtrace_symbols(array, nptrs);
    if (strings == nullptr)
    {
        LOG_ERROR(g_logger, "backtrace_symbols() error");
        throw std::exception();
    }

    for (int i = skip; i < nptrs; i++)
    {
        vec.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceXX(const int size, const int skip, const std::string& prefix)
{
    static int BT_SIZE = 64;
    void* bt_info[BT_SIZE];

    size_t bt_size = backtrace(bt_info, BT_SIZE);

    std::ostringstream ss;
    for (int i = 0; i < bt_size; i++)
    {
        ss << bt_info[i] << "\n";
    }
    std::string str = ss.str();
    return str;
}

std::string Backtrace(const int size, const int skip, const std::string& prefix)
{
    std::vector<std::string> vec;
    Backtrace(vec, size, skip);

    std::stringstream ss;
    for (const auto& str : vec)
    {
        ss << prefix << str << "\n";
    }

    return ss.str();
}

} // namespace trycle