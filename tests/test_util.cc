
#include "util.h"

#include <execinfo.h>
#include <iostream>
#include <stdio.h>
#include <string>

#include "initialize.h"
#include "macro.h"

void printf_backtrace(void)
{
    int size = 16;                                      // 堆栈的最大size
    void* buf[size];                                    // 定义用于存放获取到堆栈信息的指针数组
    int stackNum    = backtrace(buf, size);             // 返回获取到的实际堆栈信息指针数
    char** stackStr = backtrace_symbols(buf, stackNum); // 将buf中存放的信息转换为可打印的字符串信息

    for (int i = 0; i < stackNum; i++)
    {
        printf("stackNum：[%02d] stackStr：%s\n", i, stackStr[i]);
    }
    free(stackStr); // 释放一整块存放信息的字符串内存
}

void test_assert()
{
    ASSERT(false);
    ASSERT_M(1 == 2, "test 1==2");
}

void test_backtrace()
{
    printf(">>>>\n%s\n", trycle::Backtrace(64, 0, "    ").c_str());
    // std::cout << trycle::Backtrace(10, 0, "    ") << std::endl;

    test_assert();
}

int main(int argc, char** argv)
{
    printf("======================================\n");

    trycle::initialize();

    printf("--------------------------------------\n");

    test_backtrace();

    printf("--------------------------------------\n");

    // test_assert();

    printf_backtrace();

    printf("--------------------------------------\n");
}