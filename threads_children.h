#ifndef __THREADS_CHILDREN_H__
#define __THREADS_CHILDREN_H__


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "signals.h"
#include "setup.h"


#include "sched.h"

void do_thread_work(int thread_num);
int thread_function(void *argument);
void *pthread_function(void *argument);


#endif
