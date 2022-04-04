#include "sighandler.hpp"

#include <signal.h>
#include <unistd.h>
#include "misc.hpp"

static void sigbus_handler(int, siginfo_t* siginfo, void *);

void set_signal_handler(void)
{
    struct sigaction act{};
    act.sa_sigaction = sigbus_handler;
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    if(sigaction(SIGBUS, &act, nullptr) == -1){
        ERROR_THROW("sigaction(SIGBUS)");
    }
}

void sigbus_handler(int, siginfo_t* siginfo, void *)
{
    psiginfo(siginfo, progname);
    _exit(EXIT_FAILURE);
}

// vim: set expandtab shiftwidth=0 tabstop=4 :
