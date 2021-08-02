#ifndef OPTION_HPP_
#define OPTION_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

struct param{
    int mode;
};

int parse(int argc, char* argv[], param& param);

#endif // OPTION_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
