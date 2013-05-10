#ifndef __THREADS_MONITOR_H__
#define __THREADS_MONITOR_H__


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "signals.h"
#include "setup.h"


#include "units.h"
#include "sched.h"


/* thread startup, execution, handlers, etc */

void show_periodic_stats(int signum);
void show_stats_header(void);
void show_stats(void);
void store_last_stats(void);

void stop_handler(int signum);
void child_handler(int signum);

int do_monitor_work(void);

int start_threads(void);

#endif
