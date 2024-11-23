#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include <arpa/inet.h>
#include <dlfcn.h>

namespace trycle
{
static thread_local bool t_is_enable_hook;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)

bool is_enable_hook()
{
    return t_is_enable_hook;
}

void set_enable_hook(bool val)
{
    t_is_enable_hook = val;
}

void init_hook()
{
    t_is_enable_hook = false;

#define LOAD_SHARED_FUN(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(LOAD_SHARED_FUN)
#undef LOAD_SHARED_FUN
    // #define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    //     HOOK_FUN(XX);
    // #undef XX
}

struct HookIniter
{
    HookIniter()
    {
        init_hook();
    }
};

static HookIniter hook_initer;

} // namespace trycle

extern "C"
{
#define DEFINE_FUN(name) name##_fun name##_f = nullptr;
    HOOK_FUN(DEFINE_FUN)
#undef DEFINE_FUN
    // #define XX(name) name##_fun name##_f = nullptr;
    //     HOOK_FUN(XX);
    // #undef XX

    unsigned int sleep(unsigned int seconds)
    {
        if (!trycle::t_is_enable_hook)
        {
            return sleep_f(seconds);
        }

        trycle::Fiber::ptr fiber = trycle::Fiber::GetThis();
        trycle::IOManager* iom   = trycle::IOManager::GetThis();

        iom->addTimer(seconds * 1000, [fiber, iom]()
                      { iom->schedule(fiber); }, false);

        trycle::Fiber::YieldToHold();

        return 0;
    }

    int usleep(useconds_t usec)
    {
        if (!trycle::t_is_enable_hook)
        {
            return usleep_f(usec);
        }
        trycle::Fiber::ptr fiber = trycle::Fiber::GetThis();
        trycle::IOManager* iom   = trycle::IOManager::GetThis();
        iom->addTimer(usec / 1000, [fiber, iom]()
                      { iom->schedule(fiber); }, false);
        trycle::Fiber::YieldToHold();

        return 0;
    }
}
