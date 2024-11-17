#include "initialize.h"
#include "log.h"
#include "scheduler.h"
#include <chrono>

static auto g_logger = GET_LOGGER("system");

void test_func1()
{

    static int test_count = 5;
    LOG_DEBUG(g_logger, "test func1 runing....-----------" + std::to_string(test_count));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (--test_count > 0)
    {
        trycle::Scheduler::GetThis()->schedule(&test_func1);
    }
}

void test_func2()
{

    static int test_count2 = 10;
    LOG_DEBUG(g_logger, "test func2 runing...." + std::to_string(test_count2));
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    if (--test_count2 > 0)
    {
        trycle::Scheduler::GetThis()->schedule(&test_func2);
    }
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    trycle::Scheduler sc(3, false, "Scheduler-test");

    sc.start();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    sc.schedule(&test_func1);
    sc.schedule(&test_func2);
    // for (int i = 0; i < 10; i++)
    // {
    //     sc.schedule(&test_func1);
    //     sc.schedule(&test_func2);
    // }

    sc.stop();

    printf("--------------------------------------\n");
}