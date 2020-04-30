#pragma once
#define DEBUG
#ifdef DEBUG
#  include <iostream>
#  define LOG_VALUE(value) #  value << "=" << value << " "
#  define LOG_NV(name, value) name << "=" << value << " "
#  define DLOG(exp) \
    std::cout << __FILE__ << ":" << __LINE__ << "(" << errno << "):" << exp << std::endl;
#else
#  define LOG_VALUE(value)
#  define LOG_NV(name, value)
#  define DLOG(exp)
#endif
#include <sstream>
#define ERR(exp)                                                       \
  {                                                                    \
    std::ostringstream oss;                                            \
    oss << __FILE__ << ":" << __LINE__ << "(" << errno << "):" << exp; \
    throw oss.str();                                                   \
  }
