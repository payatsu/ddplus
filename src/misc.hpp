#ifndef MISC_HPP_
#define MISC_HPP_

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#define ERROR(s) \
    do{ \
        std::cerr << (s) \
            << ':' << __func__ \
            << ':' << std::to_string(__LINE__) \
            << ": " << std::strerror(errno) << std::endl; \
        return errno; \
    }while(false)

#endif // MISC_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
