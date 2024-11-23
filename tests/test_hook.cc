#include <stdint.h>
#include <string>

#include "fiber.h"
#include "hook.h"
#include "initialize.h"
#include "iomanager.h"
#include "log.h"
#include "scheduler.h"

// static auto g_logger = ;

void test_hook()
{
    trycle::IOManager iom(1);
    iom.schedule([]()
                 { 
                    LOG_DEBUG(GET_ROOT_LOGGER, "iom.addTimer(2000 start......");
                    sleep(2);
                    LOG_DEBUG(GET_ROOT_LOGGER, "iom.addTimer(2000 ......end"); });
    iom.schedule([]()
                 { 
                    LOG_DEBUG(GET_ROOT_LOGGER, "iom.addTimer(4000 start......");
                    sleep(4);
                    LOG_DEBUG(GET_ROOT_LOGGER, "iom.addTimer(4000 ......end"); });
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    test_hook();

    printf("--------------------------------------\n");

    return 0;
}