#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <memory>
#include <vector>
#include "fwd.hpp"

struct transfer{
    std::shared_ptr<target> src;
    std::shared_ptr<target> dst;
};

struct param{
    param():
        verbose(),
        width(32),
        hexdump_enabled(),
        endianness(),
        scheduling_policy(),
        jobs(1),
        repeat(1),
        transfers(){}

    bool verbose;
    int width;
    bool hexdump_enabled;
    endian endianness;
    int scheduling_policy;
    int jobs;
    int repeat;
    std::vector<transfer> transfers;
};

#endif // COMMON_HPP_

// vim: set expandtab shiftwidth=0 tabstop=4 :
