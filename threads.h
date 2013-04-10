#ifndef __THREADS_H__
#define __THREADS_H__


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "threads.h"
#include "signals.h"
#include "setup.h"


#include "units.h"
#include "sched.h"


/* thread startup, execution, handlers, etc */


void show_stats(int signum);

void stop_handler(int signum);
void child_handler(int signum);

int do_monitor_work();

/* signal to receive if my parent process dies */
void on_parent_death(int signum);

int ping_thread_function(void *argument);
void *ping_pthread_function(void *argument);

int start_threads();

#endif
