#ifndef TRY_MACRO_H
#define TRY_MACRO_H

#include "log.h"
#include "util.h"

#define ASSERT(x)                                              \
    if (!(x))                                                  \
    {                                                          \
        LOG_FMT_ERROR(GET_LOGGER("system"),                    \
                      "ASSERT: " #x ":\n%s",                   \
                      trycle::Backtrace(20, 1, "    ").c_str() \
                                                               \
        );                                                     \
    }

#define ASSERT_M(x, m)                                         \
    if (!(x))                                                  \
    {                                                          \
        LOG_FMT_ERROR(GET_LOGGER("system"),                    \
                      "ASSERT: " #x ":\n" #m "\n%s",           \
                      trycle::Backtrace(20, 1, "    ").c_str() \
                                                               \
        );                                                     \
    }

#endif // TRY_MACRO_H