#include <sched.h>
#include "misc.hpp"
#include "sched.hpp"

int to_scheduling_policy(const std::string& str)
{
    if(str == "other"){
        return SCHED_OTHER;
    }else if(str == "fifo"){
        return SCHED_FIFO;
    }else if(str == "rr"){
        return SCHED_RR;
    }else if(str == "batch"){
        return SCHED_BATCH;
    }else if(str == "iso"){
        return SCHED_ISO;
    }else if(str == "idle"){
        return SCHED_IDLE;
    }else if(str == "deadline"){
        return SCHED_DEADLINE;
    }else{
        errno = EINVAL;
        ERROR_THROW("invalid scheduling policy");
    }
}

void set_scheduling_policy(int policy)
{
    const int prio = sched_get_priority_min(policy);
    if(prio == -1){
        ERROR_THROW("", "sched_get_priority_min");
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    const struct sched_param sched_param = {
        .sched_priority = prio,
    };
#pragma GCC diagnostic pop

    if(sched_setscheduler(0, policy, &sched_param) == -1){
        ERROR_THROW("", "sched_setscheduler");
    }
}

// vim: expandtab shiftwidth=0 tabstop=4 :
