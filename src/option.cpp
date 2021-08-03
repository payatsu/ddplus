#include "option.hpp"

#include <regex>

#include <fcntl.h>
#include <getopt.h>

#include "misc.hpp"

#define REGEX_RANGE(capgrp) \
    "(" capgrp "(?:[[:digit:]]+|0x[[:xdigit:]]+)[kmgKMG]?)" "@" \
    "(" capgrp "(?:[[:digit:]]+|0x[[:xdigit:]]+)[kmgKMG]?)"

#define REGEX_TARGET \
    "((?:" REGEX_RANGE("?:") ")|(?:[[:graph:]]+))"

#define REGEX_TRANSFER \
    REGEX_TARGET ":" REGEX_TARGET

option_parser::option_parser(int argc, char* argv[])
: argc_(argc), argv_(argv) {}

int option_parser::parse_cmdopt(param& param)
{
    bool read_enabled = false;
    bool write_enabled = false;

    while(true){
        opterr = 0;
        int option_index = 0;
        option longopts[] = {
            {"read-from-ram", no_argument, nullptr, 'r'},
            {"write-to-ram",  no_argument, nullptr, 'w'},
            {}
        };

        int c = getopt_long(argc_, argv_, "rw", longopts, &option_index);
        if(c == -1){
            break;
        }

        switch(c){
        case 'r': read_enabled  = true; break;
        case 'w': write_enabled = true; break;
        case '?':
            errno = EINVAL;
            ERROR(argv_[0]);
            break;
        default:
            break;
        }
    }

    if(!(read_enabled ^ write_enabled)){
        errno = EINVAL;
        ERROR(argv_[0]);
    }

    param.mode = read_enabled ? O_RDONLY : O_WRONLY ;

    return 0;
}

int option_parser::parse_transfer(const char* str, std::string& from, std::string& to)
{
    std::cmatch m;
    if(!std::regex_match(str, m, std::regex(REGEX_TRANSFER))){
        errno = EINVAL;
        ERROR(argv_[0]);
    }

    from = m.str(1);
    to   = m.str(2);

    return 0;
}

int option_parser::parse_range(const char* str, range& range)
{
    std::cmatch m;
    if(!std::regex_match(str, m, std::regex(REGEX_RANGE("")))){
        errno = EINVAL;
        ERROR(argv_[0]);
    }

    std::size_t idx;

    range.length = std::stoul(m.str(1), &idx, 0);
    if(idx < m.str(1).size()){
        range.length *= to_number(m.str(1).at(idx));
    }
    range.base = std::stoul(m.str(2), &idx, 0);
    if(idx < m.str(2).size()){
        range.base *= to_number(m.str(2).at(idx));
    }

    return 0;
}

std::size_t option_parser::to_number(char c)
{
    std::size_t n = 1u;
    switch(c){
    case 'k': case 'K': n <<= 10; break;
    case 'm': case 'M': n <<= 20; break;
    case 'g': case 'G': n <<= 30; break;
    default: break;
    }
    return n;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
