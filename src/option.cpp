#include "option.hpp"

#include <regex>
#include <getopt.h>
#include <unistd.h>
#include "common.hpp"
#include "misc.hpp"
#include "target.hpp"

#define REGEX_RANGE(capgrp) \
    "(" capgrp "(?:[[:digit:]]+|0x[[:xdigit:]]+)[kmgKMG]?)" "@" \
    "(" capgrp "(?:[[:digit:]]+|0x[[:xdigit:]]+)[kmgKMG]?)"

#define REGEX_TARGET \
    "((?:" REGEX_RANGE("?:") ")|(?:[[:graph:]]+))"

#define REGEX_TRANSFER \
    REGEX_TARGET ":" REGEX_TARGET

option_parser::option_parser(int argc, char* argv[])
: argc_(argc), argv_(argv) {}

std::shared_ptr<param> option_parser::parse_cmdopt()const
{
    std::shared_ptr<param> prm = std::make_shared<param>();

    while(true){
        opterr = 0;
        int option_index = 0;
        option longopts[] = {
            {"help",    no_argument, nullptr, 'h'},
            {"verbose", no_argument, nullptr, 'v'},
            {"hexdump", no_argument, nullptr, 'd'},
            {}
        };

        int c = getopt_long(argc_, argv_, "dhv", longopts, &option_index);
        if(c == -1){
            break;
        }

        switch(c){
        case 'd': prm->hexdump_enabled = true; break;
        case 'h': break;
        case 'v': break;
        case '?':
            errno = EINVAL;
            ERROR_THROW(argv_[0], std::string("unknown option: '")
                    + static_cast<char>(optopt) + '\'');
            break;
        default:
            break;
        }
    }

    for(int i = optind; i < argc_; ++i){
        prm->transfers.emplace_back(to_transfer(argv_[i]));
    }

    return prm;
}

transfer option_parser::to_transfer(const std::string& spec)const
{
    std::string src, dst;
    parse_transfer(spec, src, dst);
    return transfer{
        to_target(src, target_role::SRC),
        to_target(dst, target_role::DST)
    };
}

std::shared_ptr<target> option_parser::to_target(const std::string& spec, const target_role& role)const
{
    try{
        std::size_t offset;
        std::size_t length;
        parse_range(spec, offset, length);
        return std::make_shared<target>("/dev/mem", offset, length);
    }catch(const std::runtime_error&){
        if(spec == "-"){
            return std::make_shared<target>(role == target_role::SRC ? STDIN_FILENO : STDOUT_FILENO);
        }
        return std::make_shared<target>(spec);
    }
}

void option_parser::parse_transfer(const std::string& str, std::string& src, std::string& dst)const
{
    std::smatch m;
    if(!std::regex_match(str, m, std::regex(REGEX_TRANSFER))){
        errno = EINVAL;
        ERROR_THROW(argv_[0], std::string("\"") + str + '"');
    }

    src = m.str(1);
    dst = m.str(2);
}

void option_parser::parse_range(const std::string& str, std::size_t& offset, std::size_t& length)const
{
    std::smatch m;
    if(!std::regex_match(str, m, std::regex(REGEX_RANGE("")))){
        errno = EINVAL;
        throw std::runtime_error(std::to_string(-errno));
    }

    std::size_t idx;

    length = std::stoul(m.str(1), &idx, 0);
    if(idx < m.str(1).size()){
        length *= to_number(m.str(1).at(idx));
    }
    offset = std::stoul(m.str(2), &idx, 0);
    if(idx < m.str(2).size()){
        offset *= to_number(m.str(2).at(idx));
    }
}

std::size_t option_parser::to_number(char suffix)
{
    std::size_t n = 1u;
    switch(suffix){
    case 'k': case 'K': n <<= 10; break;
    case 'm': case 'M': n <<= 20; break;
    case 'g': case 'G': n <<= 30; break;
    default: break;
    }
    return n;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
