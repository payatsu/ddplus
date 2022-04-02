#ifndef MISC_HPP_
#define MISC_HPP_

#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <iomanip>
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

class stopwatch{
    using time_point = std::chrono::high_resolution_clock::time_point;

public:
    stopwatch(bool enabled = true, const std::string& description = ""):
    enabled_(enabled), description_(description), start_(now()){}
    ~stopwatch()
    {
        if(!enabled_){
            return;
        }

        using std::chrono::duration_cast;
        using std::chrono::nanoseconds;

        time_point end = now();
        nanoseconds elapsed_time = duration_cast<nanoseconds>(end - start_);
        std::cerr << description_ << std::setprecision(6) << std::fixed
            << static_cast<double>(elapsed_time.count()) / 1000000.0
            << std::resetiosflags(std::ios::fixed) << " ms" << std::endl;
    }

private:
    static time_point now(){return std::chrono::high_resolution_clock::now();}

private:
    bool enabled_;
    const std::string description_;
    const time_point start_;
};

#endif // MISC_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
