#include "stats_periodic.h"

#include "signals.h"
#include "units.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

void show_periodic_stats() {
	static struct interval_stats_struct i_stats = { 0 };

	memset(&i_stats, 0, sizeof(struct interval_stats_struct));

	if (run_data->rusage_req_in_progress == true) /* already waiting for results */
		return;
	gather_stats(&i_stats);

	if (run_data->stop == true) /* don't worry about output...  let's just pack up and go home */
		return;

	if (run_data->stats_headers_count++ % config.stats_headers_frequency == 0)
		show_stats_header();

	show_stats(&i_stats);
	store_last_stats(&i_stats);
}
