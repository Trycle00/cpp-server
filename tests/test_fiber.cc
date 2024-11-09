#include "fiber.h"
#include "initialize.h"
#include "log.h"

void test_func1()
{
    LOG_DEBUG(GET_ROOT_LOGGER, "B33333");
    trycle::Fiber::YieldToHold();

    LOG_DEBUG(GET_ROOT_LOGGER, "C44444444444");
    trycle::Fiber::YieldToReady();
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    LOG_DEBUG(GET_ROOT_LOGGER, "A1111111111111111");

    trycle::Fiber::GetThis();
    trycle::Fiber::ptr fiber = std::make_shared<trycle::Fiber>(&test_func1);

    // test_func1();

    fiber->swap_in();

    LOG_DEBUG(GET_ROOT_LOGGER, "A2222222222222222");

    fiber->swap_in();

    LOG_DEBUG(GET_ROOT_LOGGER, "A3333333333333333");

    printf("--------------------------------------\n");
    return 0;
}