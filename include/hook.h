#ifndef TRY_HOOK_H
#define TRY_HOOK_H

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

namespace trycle
{

bool is_enable_hook();
void set_enable_hook(bool val);

} // namespace trycle

extern "C"
{

    // sleep
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun usleep_f;

    // // sleep
    // typedef unsigned int (*sleep_fun)(unsigned int seconds);
    // extern sleep_fun sleep_f;

    // typedef int (*usleep_fun)(useconds_t usec);
    // extern usleep_fun usleep_f;
}

#endif // TRY_HOOK_H
