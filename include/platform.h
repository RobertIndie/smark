#pragma once
#if defined(_MSC_VER)
#  define DISABLE_SOME_WARNINGS                                         \
    __pragma(warning(disable : 4996)) __pragma(warning(disable : 4267)) \
        __pragma(warning(disable : 4244))
#else
#  define DISABLE_SOME_WARNINGS
#endif
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#  define WIN
#endif