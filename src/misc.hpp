#ifndef MISC_HPP_
#define MISC_HPP_

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <iostream>

#define ERROR_BASE(message, finish) \
    do{ \
        std::cerr << (progname) \
            << ':' << __FILE__ \
            << ':' << std::to_string(__LINE__) \
            << ':' << __func__ \
            << ": " << std::strerror(errno) \
            << ": " << (message) << std::endl; \
        finish; \
    }while(false)

#define ERROR(message) \
    ERROR_BASE(message, return errno)

#define ERROR_THROW(message) \
    ERROR_BASE(message, throw std::runtime_error(std::strerror(errno)))

extern const char* progname;

#endif // MISC_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
