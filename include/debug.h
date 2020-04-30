#pragma once
#include <iostream>
#define DEBUG
#ifdef DEBUG
#  define LOG_VALUE(value) #  value << "=" << value << " "
#  define LOG_NV(name, value) name << "=" << value << " "
#  define DLOG(exp) \
    std::cout << __FILE__ << ":" << __LINE__ << "(" << errno << "):" exp << std::endl;
#endif
