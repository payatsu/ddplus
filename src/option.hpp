#ifndef OPTION_HPP_
#define OPTION_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <string>
#include <vector>
#include "target.hpp"

struct transfer{
    target src;
    target dst;
};

struct param{
    param():
        hexdump_enabled(),
        transfers(){}

    bool hexdump_enabled;
    std::vector<transfer> transfers;
};

struct range{
    std::size_t offset;
    std::size_t length;
};

enum class target_role{
    SRC,
    DST,
};

class option_parser{
public:
    option_parser(int argc, char* argv[]);

    param parse_cmdopt()const;

    transfer to_transfer(const std::string& spec)const;
    target to_target(const std::string& spec, target_role role)const;

    void parse_transfer(const std::string& str, std::string& src, std::string& dst)const;
    void parse_range(const std::string& str, range& range)const;

private:
    static std::size_t to_number(char suffix);

private:
    int argc_;
    char** argv_;
};

#endif // OPTION_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
