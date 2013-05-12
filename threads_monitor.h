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

struct interval_stats_struct {
	unsigned long long current_count;
	unsigned long long interval_count;
	long double current_time;
	long double interval_time;
	long double run_time;
	struct rusage rusage[2];
	unsigned long long interval_tsc[2];

	long csw[2]; /* per-thread per-interval csw */

	long double iteration_time; /* time for each iteration */
	long double cpi[2]; /* cycles-per-iteration per CPU */
	long double mhz[2]; /* approximate speed of the CPU */
};

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
