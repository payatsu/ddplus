#ifndef SCHED_HPP_
#define SCHED_HPP_

#include <string>

int to_scheduling_policy(const std::string& str);

void set_scheduling_policy(int policy);

#endif // SCHED_HPP_

// vim: set expandtab shiftwidth=0 tabstop=4 :
