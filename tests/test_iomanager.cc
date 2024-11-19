#include "initialize.h"
#include "iomanager.h"

static auto g_logger = GET_LOGGER("system");

void test_fiber()
{
    LOG_DEBUG(g_logger, "test_fiber runing.............");
}

void test_iomanager()
{
    trycle::IOManager iom;
    iom.schedule(&test_fiber);
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    test_iomanager();

    printf("--------------------------------------\n");

    return 0;
}