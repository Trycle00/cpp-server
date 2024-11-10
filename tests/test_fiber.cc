#include "fiber.h"
#include "initialize.h"
#include "log.h"

using namespace trycle;

auto g_logger = GET_LOGGER("system");

void run_in_fiber()
{
    LOG_DEBUG(g_logger, "run_in_fiber begin");
    Fiber::YieldToHold();
    LOG_DEBUG(g_logger, "run_in_fiber end");
    Fiber::YieldToHold();
}

void test_fiber()
{
    LOG_DEBUG(g_logger, "main begin -1");
    {
        Fiber::GetThis();
        LOG_DEBUG(g_logger, "main begin");
        Fiber::ptr fiber(new Fiber(run_in_fiber));
        fiber->swap_in();
        LOG_DEBUG(g_logger, "main after swap_in");
        fiber->swap_in();
        LOG_DEBUG(g_logger, "main after end");
        fiber->swap_in();
    }
    LOG_DEBUG(g_logger, "main after end2");
}

void test_func1()
{
    LOG_DEBUG(g_logger, "B33333");
    trycle::Fiber::YieldToHold();

    LOG_DEBUG(g_logger, "C44444444444");
    trycle::Fiber::YieldToHold();
}

void test_fiber1()
{
    LOG_DEBUG(g_logger, "A1111111111111111");
    {
        trycle::Fiber::GetThis();

        trycle::Fiber::ptr fiber = std::make_shared<trycle::Fiber>(test_func1);

        fiber->swap_in();

        LOG_DEBUG(g_logger, "A2222222222222222--swap_in-1");

        fiber->swap_in();

        LOG_DEBUG(g_logger, "A2222222222222222--swap_in-2");

        fiber->swap_in(); // 最后需要回到fiber中
    }
    LOG_DEBUG(g_logger, "A3333333333333333...");
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    test_fiber1();

    printf("--------------------------------------\n");

    // test_fiber();

    printf("--------------------------------------\n");
    return 0;
}