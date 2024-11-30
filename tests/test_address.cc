/*
 * @Author: trycle
 * @Date: 2024-11-29 02:33:55
 * @LastEditors: trycle
 * @LastEditTime: 2024-11-30 18:01:31
 * @Description: 请填写简介
 */
#include <map>
#include <stdint.h>
#include <vector>

#include "address.h"
#include "initialize.h"
#include "log.h"

void test_ipv4()
{

    auto addr = trycle::IpAddress::Create("127.0.0.1");
    if (addr)
    {
        LOG_DEBUG(GET_ROOT_LOGGER, addr->toString());
    }
}

void test_iface()
{
    std::multimap<std::string, std::pair<trycle::Address::ptr, uint32_t>> results;

    auto rt = trycle::Address::GetInterfaceAddress(results);
    if (!rt)
    {
        LOG_ERROR(GET_ROOT_LOGGER, "GetInterfaceAddress failed");
        return;
    }
    for (auto& item : results)
    {
        LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s -- %s -- %d",
                      item.first.c_str(), item.second.first->toString().c_str(), item.second.second);
    }
}

void test()
{
    std::vector<trycle::Address::ptr> addrs;

    bool rt = trycle::Address::Lookup(addrs, "192.168.0.180:11");
    if (!rt)
    {
        LOG_ERROR(GET_ROOT_LOGGER, "lookup failed");
        return;
    }
    for (size_t i = 0; i < addrs.size(); ++i)
    {
        LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%ld -- %s", i, addrs[i]->toString().c_str());
    }

    printf("-------------\n");

    auto addr = trycle::Address::LookupAny("192.168.0.180:33306");
    if (addr)
    {
        LOG_FMT_DEBUG(GET_ROOT_LOGGER, "addr=%s", addr->toString().c_str());
    }
    else
    {
        LOG_ERROR(GET_ROOT_LOGGER, "LookupAny failed");
    }
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    test_ipv4();

    printf("--------------------------------------\n");

    test_iface();

    printf("--------------------------------------\n");

    test();

    printf("--------------------------------------\n");

    return 0;
}
