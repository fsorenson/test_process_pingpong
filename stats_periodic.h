#ifndef __STATS_PERIODIC_H__
#define __STATS_PERIODIC_H__

#include "test_process_pingpong.h"
#include "stats_common.h"

int gather_periodic_stats(struct interval_stats_struct *i_stats);
void show_periodic_stats(void);
void show_periodic_stats_header(void);
void show_periodic_stats_data(struct interval_stats_struct *i_stats);
void store_last_stats(struct interval_stats_struct *i_stats);

#endif
