#include "option.hpp"

#include <fcntl.h>
#include <getopt.h>

#include "misc.hpp"

int parse(int argc, char* argv[], param& param)
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

        int c = getopt_long(argc, argv, "rw", longopts, &option_index);
        if(c == -1){
            break;
        }

        switch(c){
        case 'r': read_enabled  = true; break;
        case 'w': write_enabled = true; break;
        case '?':
            errno = EINVAL;
            ERROR(argv[0]);
            break;
        default:
            break;
        }
    }

    if(!(read_enabled ^ write_enabled)){
        errno = EINVAL;
        ERROR(argv[0]);
    }

    param.mode = read_enabled ? O_RDONLY : O_WRONLY ;

    return 0;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
