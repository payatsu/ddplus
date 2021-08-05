#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdexcept>
#include "common.hpp"
#include "option.hpp"
#include "target.hpp"

int main(int argc, char* argv[])
{
    try{
        std::shared_ptr<param> param = option_parser(argc, argv).parse_cmdopt();
        for(const auto& [src, dst]: param->transfers){
            src->transfer_to(*dst, *param);
        }
    }catch(const std::runtime_error&){
        ; // do nothing.
    }

    return 0;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
