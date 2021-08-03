#ifndef MISC_HPP_
#define MISC_HPP_

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <string>

#define ERROR_BASE(progname, message, finish) \
    do{ \
        std::cerr << (progname) \
            << ':' << __FILE__ \
            << ':' << std::to_string(__LINE__) \
            << ':' << __func__ \
            << ": " << std::strerror(errno) \
            << ": " << (message) << std::endl; \
        finish; \
    }while(false)

#define ERROR(progname, message) \
    ERROR_BASE(progname, message, return errno)

#define ERROR_THROW(progname, message) \
    ERROR_BASE(progname, message, throw std::runtime_error(std::strerror(errno)))

#endif // MISC_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
