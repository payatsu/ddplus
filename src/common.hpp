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
        width(32),
        hexdump_enabled(),
        endianness(),
        scheduling_policy(),
        transfers(){}

    int width;
    bool hexdump_enabled;
    endian endianness;
    int scheduling_policy;
    std::vector<transfer> transfers;
};

#endif // COMMON_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
