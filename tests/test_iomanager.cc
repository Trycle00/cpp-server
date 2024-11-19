#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "initialize.h"
#include "iomanager.h"
#include "macro.h"

static auto g_logger = GET_LOGGER("system");

void test_fiber()
{
    LOG_DEBUG(g_logger, "test_fiber runing.............");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(sock != -1);

    fcntl(sock, F_SETFL, nullptr, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(80);
    inet_pton(AF_INET, "110.242.68.66", &addr.sin_addr.s_addr);

    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    if (!rt)
    {
        LOG_FMT_DEBUG(g_logger, "success | rt=%d", rt);
        trycle::IOManager::GetThis()->addEvent(sock, trycle::IOManager::EventType::READ, []()
                                               { LOG_DEBUG(g_logger, "handle read event."); });

        trycle::IOManager::GetThis()->addEvent(sock, trycle::IOManager::EventType::WRITE, []()
                                               { LOG_DEBUG(g_logger, "handle write event."); });
    }
    else if (errno == EINPROGRESS)
    {
        LOG_FMT_ERROR(g_logger, "Add event error | errno=%d, strerror=%s", errno, strerror(errno));

        trycle::IOManager::GetThis()->addEvent(sock, trycle::IOManager::EventType::WRITE, []()
                                               { LOG_DEBUG(g_logger, "Connected"); });
    }
    else
    {
        LOG_FMT_ERROR(g_logger, "other error | errno=%d, strerror=%s", errno, strerror(errno));
    }
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