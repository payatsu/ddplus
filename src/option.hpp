#ifndef OPTION_HPP_
#define OPTION_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <memory>
#include <string>
#include "fwd.hpp"

class option_parser{
public:
    option_parser(int argc, char* argv[]);

    std::shared_ptr<param> parse_cmdopt()const;

    transfer to_transfer(const std::string& spec)const;
    std::shared_ptr<target> to_target(const std::string& spec, const target_role& role)const;

    void parse_transfer(const std::string& str, std::string& src, std::string& dst)const;
    void parse_range(const std::string& str, std::size_t& offset, std::size_t& length)const;

private:
    static std::size_t to_number(char suffix);
    static int to_repeat(const std::string& spec);

private:
    int argc_;
    char** argv_;
};

#endif // OPTION_HPP_

// vim: set expandtab shiftwidth=0 tabstop=4 :
