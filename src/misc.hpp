#ifndef MISC_HPP_
#define MISC_HPP_

#include <cerrno>
#include <chrono>
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

class stopwatch{
    using time_point = std::chrono::high_resolution_clock::time_point;

public:
    stopwatch(const std::string& description = ""): description_(description), start_(now()){}
    ~stopwatch()
    {
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;

        time_point end = now();
        milliseconds elapsed_time = duration_cast<milliseconds>(end - start_);
        std::cerr << description_ << elapsed_time.count() << " ms" << std::endl;
    }

private:
    static time_point now(){return std::chrono::high_resolution_clock::now();}

private:
    const std::string description_;
    const time_point start_;
};

#endif // MISC_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
