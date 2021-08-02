#include "option.hpp"

#include <regex>

#include <fcntl.h>
#include <getopt.h>

#include "misc.hpp"

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

int option_parser::parse_range(const char* str, range& range)
{
    std::cmatch m;
    if(!std::regex_match(str, m, std::regex(
            "(([[:digit:]]+|0x[[:xdigit:]]+)[kmgKMG]?)" ":"
            "(([[:digit:]]+|0x[[:xdigit:]]+)[kmgKMG]?)"
        ))){
        errno = EINVAL;
        ERROR(argv_[0]);
    }

    std::size_t idx;

    auto base = std::stoull(m.str(1), &idx, 0);
    if(idx < m.str(1).size()){
        base = multiply_by_suffix(base, m.str(1).at(idx));
    }
    auto length = std::stoull(m.str(3), &idx, 0);
    if(idx < m.str(3).size()){
        length = multiply_by_suffix(length, m.str(3).at(idx));
    }

    range.base   = base;
    range.length = length;
    return 0;
}

std::size_t option_parser::multiply_by_suffix(std::size_t value, char suffix)
{
    switch(suffix){
    case 'k':
    case 'K':
        value <<= 10;
        break;
    case 'm':
    case 'M':
        value <<= 20;
        break;
    case 'g':
    case 'G':
        value <<= 30;
        break;
    default:
        break;
    }
    return value;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
