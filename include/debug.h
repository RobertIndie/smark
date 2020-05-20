#pragma once
#pragma warning(disable:4996)
#include <errno.h>
#include <string.h>
#define PRINT_ERRNO errno << " " << strerror(errno)
#define DEBUG
#ifdef DEBUG
#  include <iostream>
// public:
#  define LOG_VALUE(value) #  value << "=" << value << " "
#  define LOG_NV(name, value) name << "=" << value << " "
#  define DLOG(exp) \
    OUTPUT_STREAM << __FILE__ << ":" << __LINE__ << "(" << PRINT_ERRNO << "):" << exp << std::endl;
// private
#  define OUTPUT_STREAM std::cout
#else
#  define LOG_VALUE(value)
#  define LOG_NV(name, value)
#  define DLOG(exp)
#endif
#include <sstream>
#define ERR(exp)            \
  {                         \
    std::ostringstream oss; \
    oss << exp;             \
    std::cout << "Error:";  \
    DLOG(oss.str())         \
    throw oss.str();        \
  }
