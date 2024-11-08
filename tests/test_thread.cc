#include "initialize.h"
#include "log.h"
#include "thread.h"
#include <string>
#include <vector>

int counter = 0;
trycle::RWMutex s_mutex;

void test_fun1()
{
    printf("xxxxxxx\n");
    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "name=%s, this.name=%s, pid=%ld, this.pid=%d",
                  trycle::Thread::get_current_name().c_str(),
                  trycle::Thread::get_current_thread()->get_name().c_str(),
                  trycle::GetThreadId(),
                  trycle::Thread::get_current_thread()->get_id());

    for (int i = 0; i < 10000000; i++)
    {
        trycle::RWMutex::WriteLock lock(s_mutex);
        counter++;
    }
}

int main(int argc, char** argv)
{
    trycle ::initialize();

    printf("====================================\n");

    std::vector<trycle::Thread::ptr> threads;
    for (size_t i = 0; i < 5; i++)
    {
        auto thread = std::make_shared<trycle::Thread>("name-" + std::to_string(i), &test_fun1);
        threads.push_back(thread);
    }

    for (const auto& item : threads)
    {
        item->join();
    }

    printf("------------------------------------\n");

    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "counter=%d", counter);

    printf("------------------------------------\n");
    return 0;
}