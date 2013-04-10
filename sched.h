#ifndef __SCHED_H__
#define __SCHED_H__

#include "test_process_pingpong.h"
#include <sched.h>

int get_priorities();
int set_priorities();

int get_sched_interval();

int parse_sched(char *arg);

int sched_getcpu();
int num_cpus();
int num_online_cpus();


#endif
