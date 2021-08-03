#ifndef OPTION_HPP_
#define OPTION_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <string>

struct param{
    int mode;
};

struct range{
    std::size_t base;
    std::size_t length;
};

class option_parser{
public:
    option_parser(int argc, char* argv[]);
    int parse_cmdopt(param& param);
    int parse_transfer(const char* str, std::string& from, std::string& to);
    int parse_range(const char* str, range& range);

private:
    static std::size_t to_number(char c);

private:
    int argc_;
    char** argv_;
};

#endif // OPTION_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
