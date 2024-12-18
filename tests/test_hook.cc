#include <arpa/inet.h>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "fiber.h"
#include "hook.h"
#include "initialize.h"
#include "iomanager.h"
#include "log.h"
#include "scheduler.h"

// static auto g_logger = GET_ROOT_LOGGER;

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

void test_socket()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(80);
    inet_pton(AF_INET, "39.156.66.10", &addr.sin_addr.s_addr);

    LOG_DEBUG(GET_ROOT_LOGGER, "connect start...");
    int rt = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "connect finish | rt=%d, errno=%d", rt, errno);

    if (rt)
    {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt                = send(sock, data, sizeof(data), 0);
    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "send finish | rt=%d, errno=%d", rt, errno);
    if (rt <= 0)
    {
        return;
    }

    std::string buff;
    buff.resize(40960);

    rt = recv(sock, &buff[0], buff.size(), 0);
    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "recv finish | rt=%d, errno=%d", rt, errno);
    if (rt <= 0)
    {
        return;
    }

    buff.resize(rt);
    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "RECV result :\n%s", buff.c_str());
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    LOG_DEBUG(GET_ROOT_LOGGER, "TTTTTTTTTTTTTTTTTTT...");

    // test_hook();

    trycle::IOManager iom(2);
    iom.schedule(&test_socket);

    printf("--------------------------------------\n");

    return 0;
}