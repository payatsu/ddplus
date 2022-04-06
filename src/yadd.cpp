#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include "common.hpp"
#include "misc.hpp"
#include "option.hpp"
#include "target.hpp"

const char* progname = nullptr;

int main(int argc, char* argv[])
{
    progname = argv[0];
    stopwatch sw(std::string(progname) + ": ");

    try{
        std::shared_ptr<param> param = option_parser(argc, argv).parse_cmdopt();
        sw.set(param->verbose);
        for(int i = 0; i < param->repeat || param->repeat < 0; ++i){
            for(const auto& [src, dst]: param->transfers){
                src->transfer_to(*dst, *param);
            }
        }
    }catch(const std::exception&){
        ; // do nothing.
    }

    return 0;
}

// vim: set expandtab shiftwidth=0 tabstop=4 :
