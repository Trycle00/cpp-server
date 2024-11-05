#include <iostream>

#include "config.h"

trycle::ConfigVar<int>::ptr int_val = trycle::Config::lookUp("test.int_val", 1);

int main(int argc, char** argv)
{
    std::cout << "hello....." << std::endl;
}