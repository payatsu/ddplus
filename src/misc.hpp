#ifndef MISC_HPP_
#define MISC_HPP_

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#define ERROR(progname, message) \
    do{ \
        std::cerr << (progname) \
            << ':' << __FILE__ \
            << ':' << std::to_string(__LINE__) \
            << ':' << __func__ \
            << ": " << std::strerror(errno) \
            << ": " << (message) << std::endl; \
        return errno; \
    }while(false)

#endif // MISC_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
