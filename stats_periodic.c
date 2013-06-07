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

void show_stats_header(void) {
#define OUTPUT_BUFFER_LEN 400
	static char output_buffer[OUTPUT_BUFFER_LEN];
	size_t output_buffer_len = OUTPUT_BUFFER_LEN;
#undef OUTPUT_BUFFER_LEN

	// original format
	// 83.000 s - 64962 iterations -> 15.393 us, ping: 40945.64 cycles, pong: 40945.100 cycles

	// header
	snprintf(output_buffer, output_buffer_len,
		"%7s %12s %11s", "", "", "");
	snprintf(output_buffer + strlen(output_buffer), output_buffer_len - strlen(output_buffer),
		" %s %s",
		" ___________ PING ___________ ",
		" ___________ PONG ___________ "
		);
	write(1, output_buffer, strlen(output_buffer));
	write(1, "\n", 1);

	snprintf(output_buffer, output_buffer_len,
		"%7s %12s %11s",
		"time", "cycles/sec", "cycle time");

	// per-thread header
	snprintf(output_buffer + strlen(output_buffer), output_buffer_len - strlen(output_buffer),
		" | %13s  %5s  %5s",
		" vol/inv csw", "user", "sys");
	snprintf(output_buffer + strlen(output_buffer), output_buffer_len - strlen(output_buffer),
		" | %13s  %5s  %5s",
		" vol/inv csw", "user", "sys");
	write(1, output_buffer, strlen(output_buffer));

	write(1, "\n", 1);
}
