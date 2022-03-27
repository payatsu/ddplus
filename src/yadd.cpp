#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include "common.hpp"
#include "option.hpp"
#include "target.hpp"

const char* progname = nullptr;

int main(int argc, char* argv[])
{
    progname = argv[0];

    try{
        std::shared_ptr<param> param = option_parser(argc, argv).parse_cmdopt();
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

// vim: expandtab shiftwidth=0 tabstop=4 :
