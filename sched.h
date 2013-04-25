#ifndef __SCHED_H__
#define __SCHED_H__

#include "test_process_pingpong.h"
#include <sched.h>

int get_priorities(void);
int set_priorities(void);

int get_sched_interval(void);

int parse_sched(char *arg);

#ifndef HAVE_SCHED_GETCPU
int sched_getcpu(void);
#endif

int num_cpus(void);
int num_online_cpus(void);


#endif
