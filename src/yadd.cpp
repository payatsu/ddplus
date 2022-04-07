#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include <sys/resource.h>
#include "common.hpp"
#include "misc.hpp"
#include "option.hpp"
#include "target.hpp"

const char* progname = nullptr;

int main(int argc, char* argv[])
{
    progname = argv[0];
    stopwatch sw(std::string(progname) + ": ");

    struct rlimit rlim;
    if(getrlimit(RLIMIT_CORE, &rlim)   == -1 ||
       (rlim.rlim_cur = rlim.rlim_max) ==  0 ||
       setrlimit(RLIMIT_CORE, &rlim)   == -1){
        ERROR("getrlimit/setrlimit");
    }

    try{
        std::shared_ptr<param> param = option_parser(argc, argv).parse_cmdopt();
        sw.set(param->verbose);
        for(int i = 0; i < param->repeat || param->repeat < 0; ++i){
            for(const auto& [src, dst]: param->transfers){
                src->transfer_to(*dst, *param);
            }
        }
    }catch(const std::exception&){
        sw.set(false);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim: set expandtab shiftwidth=0 tabstop=4 :
