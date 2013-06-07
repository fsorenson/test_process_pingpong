#ifndef __STATS_PERIODIC_H__
#define __STATS_PERIODIC_H__

#include "test_process_pingpong.h"

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

void show_periodic_stats(void);
void show_stats_header(void);
void show_stats(struct interval_stats_struct *i_stats);
void store_last_stats(struct interval_stats_struct *i_stats);

#endif
