#ifndef __THREADS_MONITOR_H__
#define __THREADS_MONITOR_H__

#include "test_process_pingpong.h"
#include "signals.h"
#include "setup.h"

#include "units.h"
#include "sched.h"


/* thread startup, execution, handlers, etc */

void show_periodic_stats(int signum);
void show_stats_header(void);
void show_stats(struct interval_stats_struct *i_stats);
void store_last_stats(struct interval_stats_struct *i_stats);

void stop_handler(int signum);
void child_handler(int signum);

int do_monitor_work(void);

#endif
