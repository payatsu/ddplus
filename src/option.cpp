#include "option.hpp"

#include <regex>
#include <getopt.h>
#include "sched.hpp"
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

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "THIS-TOOL"
#endif

#ifndef PACKAGE_STRING
#define PACKAGE_STRING PACKAGE_NAME
#endif

static void show_version(void)
{
    std::puts(PACKAGE_STRING
#ifdef GIT_COMMIT_ID
            " git:" GIT_COMMIT_ID
#endif
            R"(
                               __            __
         ____ ___  ____ ______/ /____  _____/ /_____  __  __
        / __ `__ \/ __ `/ ___/ __/ _ \/ ___/ //_/ _ \/ / / /
       / / / / / / /_/ (__  ) /_/  __/ /  / ,< /  __/ /_/ /
      /_/ /_/ /_/\__,_/____/\__/\___/_/  /_/|_|\___/\__, /
                                                   /____/
)"
            );
    std::exit(EXIT_SUCCESS);
}

static void show_help(void)
{
    std::printf(
            "Usage: " PACKAGE_NAME " [OPTIONS]... TRANSFERS\n"
            R"(Transfer data from source to destination.

Options:
    -h, --help              show this help and exit.
    -V, --version           show version and exit.
    -v, --verbose           turn on verbose output.
    -d, --hexdump           output hexadecimal style, instead of raw style.
    -w NUM, --width NUM     access bit width. NUM is either of 8, 16, 32, 64.
                            by default, 32 is used.
    -e TYPE, --endian TYPE  specify endian while hexdump('-d').
                            TYPE is either of host, big, little.
                            by default, host is used.
    -s POLICY,              specify thread scheduling policy.
    --schedule POLICY       POLICY is either of
                            other, fifo, rr, batch, iso, idle, deadline.
    -j N, --jobs N          allow N threads at once.
    -r COUNT,               specify repeat count.
    --repeat COUNT          COUNT is a integer greater than zero, or endless.

Syntax:
    TRANSFERS       :=  TRANSFER [ " " TRANSFERS ]
    TRANSFER        :=  SRC ":" DST

    SRC             :=  { LENGTH "@" OFFSET | "-" | path-to-a-existing-file }
    DST             :=  { LENGTH "@" OFFSET | "-" | path-to-a-output-file }
                        LENGTH@OFFSET represents a physical address region
                        which starts at OFFSET and has length LENGTH.
                        "-" represents stdin  in SRC context.
                        "-" represents stdout in DST context.
                        note that when you use stdin you need a preceding
                        "--", which means end of options, to avoid confusion.

    LENGTH, OFFSET  :=  {      decimal-digit's
                        | "0x"     hex-digit's
                        |  "0"   octal-digit's } [ SUFFIX ]
    SUFFIX          :=  { "k" | "K"
                        | "m" | "M"
                        | "g" | "G" }
                        k/K: 1024
                        m/M: 1024 * 1024
                        g/G: 1024 * 1024 * 1024
)"
            );
    std::exit(EXIT_SUCCESS);
}

option_parser::option_parser(int argc, char* argv[])
: argc_(argc), argv_(argv) {}

std::shared_ptr<param> option_parser::parse_cmdopt()const
{
    std::shared_ptr<param> prm = std::make_shared<param>();

    while(true){
        opterr = 0;
        int option_index = 0;
        option longopts[] = {
            {"help",           no_argument, nullptr, 'h'},
            {"version",        no_argument, nullptr, 'V'},
            {"verbose",        no_argument, nullptr, 'v'},
            {"hexdump",        no_argument, nullptr, 'd'},
            {"width",    required_argument, nullptr, 'w'},
            {"endian",   required_argument, nullptr, 'e'},
            {"schedule", required_argument, nullptr, 's'},
            {"jobs",     required_argument, nullptr, 'j'},
            {"repeat",   required_argument, nullptr, 'r'},
            {}
        };

        int c = getopt_long(argc_, argv_, "Vde:hj:r:s:vw:", longopts, &option_index);
        if(c == -1){
            break;
        }

        switch(c){
        case 'V': show_version(); break;
        case 'd': prm->hexdump_enabled = true; break;
        case 'e': prm->endianness = to_endian(optarg); break;
        case 'h': show_help(); break;
        case 'j':
            try{
                prm->jobs = std::stoi(optarg, nullptr, 0);
            }catch(const std::exception& e){
                errno = EINVAL;
                ERROR_THROW(std::string("can't convert to number: '")
                        + optarg + "'");
            }
            if(prm->jobs < 1){
                errno = EINVAL;
                ERROR_THROW(std::string("invalid value: ")
                        + std::to_string(prm->jobs));
            }
            break;
        case 'r': prm->repeat = to_repeat(optarg); break;
        case 's': prm->scheduling_policy = to_scheduling_policy(optarg); break;
        case 'v': prm->verbose = true; break;
        case 'w':
            try{
                prm->width = static_cast<decltype(prm->width)>(std::stol(optarg));
            }catch(const std::exception& e){
                errno = EINVAL;
                ERROR_THROW(std::string("can't convert to number: '")
                        + optarg + "'");
            }
            switch(prm->width){
            case 8: case 16: case 32: case 64: break;
            default:
                errno = EINVAL;
                ERROR_THROW(std::string("invalid value: ")
                        + std::to_string(prm->width));
                break;
            }
            break;
        case '?':
            errno = EINVAL;
            ERROR_THROW(std::string("unknown option: '")
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
    std::size_t offset;
    std::size_t length;
    try{
        parse_range(spec, offset, length);
    }catch(const std::runtime_error&){
        if(spec == "-"){
            return std::make_shared<target>(role == target_role::SRC ? STDIN_FILENO : STDOUT_FILENO);
        }
        return std::make_shared<target>(spec, role);
    }
    return std::make_shared<target>("/dev/mem", role, offset, length);
}

void option_parser::parse_transfer(const std::string& str, std::string& src, std::string& dst)const
{
    std::smatch m;
    if(!std::regex_match(str, m, std::regex(REGEX_TRANSFER))){
        errno = EINVAL;
        ERROR_THROW(std::string("\"") + str + '"');
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

int option_parser::to_repeat(const std::string& spec)
{
    if(spec == "endless"){
        return -1;
    }

    int n = 0;
    try{
        n = std::stoi(spec, nullptr, 0);
    }catch(const std::exception& e){
        errno = EINVAL;
        ERROR_THROW(std::string("can't convert to number: '")
                + spec + "'");
    }

    if(n <= 0){
        errno = EINVAL;
        ERROR_THROW("repeat number must be greater than zero: '" + spec + "'");
    }
    return n;
}

// vim: set expandtab shiftwidth=0 tabstop=4 :
