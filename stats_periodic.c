#include "stats_periodic.h"

#include "signals.h"
#include "units.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

int gather_periodic_stats(struct interval_stats_struct *i_stats) {

	i_stats->current_count = run_data->ping_count;
	i_stats->current_time = get_time();

	run_data->rusage_req_in_progress = true;
	run_data->rusage_req[0] = true;
	run_data->rusage_req[1] = true;
	send_sig(run_data->thread_info[0].pid, SIGUSR1);
	send_sig(run_data->thread_info[1].pid, SIGUSR2);

	i_stats->run_time = i_stats->current_time - run_data->start_time;
	i_stats->interval_time = i_stats->current_time - run_data->last_stats_time;
	i_stats->interval_count = i_stats->current_count - run_data->last_ping_count;


	while (run_data->rusage_req_in_progress == true) {
		if ((run_data->rusage_req[0] == false) && (run_data->rusage_req[1] == false))
			run_data->rusage_req_in_progress = false;
		if (run_data->rusage_req_in_progress == true)
			__sync_synchronize();
		if (run_data->stop == true)
			return -1;
	}

	i_stats->rusage[0].ru_nvcsw = run_data->thread_stats[0].rusage.ru_nvcsw - run_data->thread_stats[0].last_rusage.ru_nvcsw;
	i_stats->rusage[0].ru_nivcsw = run_data->thread_stats[0].rusage.ru_nivcsw - run_data->thread_stats[0].last_rusage.ru_nivcsw;
	i_stats->rusage[1].ru_nvcsw = run_data->thread_stats[1].rusage.ru_nvcsw - run_data->thread_stats[1].last_rusage.ru_nvcsw;
	i_stats->rusage[1].ru_nivcsw = run_data->thread_stats[1].rusage.ru_nivcsw - run_data->thread_stats[1].last_rusage.ru_nivcsw;

	i_stats->csw[0] = i_stats->rusage[0].ru_nvcsw + i_stats->rusage[0].ru_nivcsw;
	i_stats->csw[1] = i_stats->rusage[1].ru_nvcsw + i_stats->rusage[1].ru_nivcsw;

	i_stats->rusage[0].ru_utime = elapsed_time_timeval(run_data->thread_stats[0].last_rusage.ru_utime, run_data->thread_stats[0].rusage.ru_utime);
	i_stats->rusage[0].ru_stime = elapsed_time_timeval(run_data->thread_stats[0].last_rusage.ru_stime, run_data->thread_stats[0].rusage.ru_stime);

	i_stats->rusage[1].ru_utime = elapsed_time_timeval(run_data->thread_stats[1].last_rusage.ru_utime, run_data->thread_stats[1].rusage.ru_utime);
	i_stats->rusage[1].ru_stime = elapsed_time_timeval(run_data->thread_stats[1].last_rusage.ru_stime, run_data->thread_stats[1].rusage.ru_stime);

	if (config.set_affinity == true) {
		i_stats->interval_tsc[0] = run_data->thread_stats[0].tsc - run_data->thread_stats[0].last_tsc;
		i_stats->interval_tsc[1] = run_data->thread_stats[1].tsc - run_data->thread_stats[1].last_tsc;
		i_stats->mhz[0] = i_stats->interval_tsc[0] / i_stats->interval_time / 1000 / 1000;
		i_stats->mhz[1] = i_stats->interval_tsc[1] / i_stats->interval_time / 1000 / 1000;

		i_stats->cpi[0] = (long double)i_stats->interval_tsc[0] / (long double)i_stats->interval_count;
		i_stats->cpi[1] = (long double)i_stats->interval_tsc[1] / (long double)i_stats->interval_count;
	} else {
		i_stats->interval_tsc[0] = i_stats->interval_tsc[1] = 0;
		i_stats->mhz[0] = i_stats->mhz[1] = 0;
		i_stats->cpi[0] = i_stats->cpi[1] = 0;
	}

	i_stats->iteration_time = i_stats->interval_time / i_stats->interval_count;

	return 0;
}


void show_periodic_stats(void) {
	static struct interval_stats_struct i_stats = { 0 };

	memset(&i_stats, 0, sizeof(struct interval_stats_struct));

	if (run_data->rusage_req_in_progress == true) /* already waiting for results */
		return;
	gather_periodic_stats(&i_stats);

	if (run_data->stop == true) /* don't worry about output...  let's just pack up and go home */
		return;

	if (run_data->stats_headers_count++ % config.stats_headers_frequency == 0)
		show_periodic_stats_header();

	show_periodic_stats_data(&i_stats);
	store_last_stats(&i_stats);
}

void show_periodic_stats_header(void) {
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

void show_periodic_stats_data(struct interval_stats_struct *i_stats) {
#define OUTPUT_BUFFER_LEN 400
	static char output_buffer[OUTPUT_BUFFER_LEN] = { 0 };
	size_t output_buffer_len = OUTPUT_BUFFER_LEN;
#undef OUTPUT_BUFFER_LEN
#define TEMP_STRING_LEN 30
	static char temp_string1[TEMP_STRING_LEN] = { 0 };
	static char temp_string2[TEMP_STRING_LEN] = { 0 };
	size_t temp_string_len = TEMP_STRING_LEN;
#undef TEMP_STRING_LEN
	integer_fixed_point_t int_fp1, int_fp2;

	memset(output_buffer, 0, output_buffer_len);
	memset(temp_string1, 0, temp_string_len);
	memset(temp_string2, 0, temp_string_len);

	// general stats
	snprintf(output_buffer, output_buffer_len, "%7s %12llu %11s",
		subsec_string(temp_string1, i_stats->run_time, 1),
		(unsigned long long)((long double)i_stats->interval_count / i_stats->interval_time),
		subsec_string(temp_string2, i_stats->iteration_time, 2));
	write(1, output_buffer, strlen(output_buffer));

	// per-thread stats
	snprintf(output_buffer, output_buffer_len, " | %5ld / %5ld",
		i_stats->rusage[0].ru_nvcsw, i_stats->rusage[0].ru_nivcsw);
	write(1, output_buffer, strlen(output_buffer));

	int_fp1 = f_to_fp(1, ((i_stats->rusage[0].ru_utime.tv_sec * 100.0L) + (i_stats->rusage[0].ru_utime.tv_usec / 1.0e4L) / i_stats->interval_time));
	int_fp2 = f_to_fp(1, ((i_stats->rusage[0].ru_stime.tv_sec * 100.0L) + (i_stats->rusage[0].ru_stime.tv_usec / 1.0e4L) / i_stats->interval_time));

	snprintf(output_buffer, output_buffer_len, "  %2lu.%01lu%%  %2lu.%01lu%%",
		int_fp1.i, int_fp1.dec, int_fp2.i, int_fp2.dec);
	write(1, output_buffer, strlen(output_buffer));

	snprintf(output_buffer, output_buffer_len, " | %5ld / %5ld",
		i_stats->rusage[1].ru_nvcsw, i_stats->rusage[1].ru_nivcsw);
	write(1, output_buffer, strlen(output_buffer));

	int_fp1 = f_to_fp(1, ((i_stats->rusage[1].ru_utime.tv_sec * 100.0L) + (i_stats->rusage[1].ru_utime.tv_usec / 1.0e4L) / i_stats->interval_time));
	int_fp2 = f_to_fp(2, ((i_stats->rusage[1].ru_stime.tv_sec * 100.0L) + (i_stats->rusage[1].ru_stime.tv_usec / 1.0e4L) / i_stats->interval_time));

	snprintf(output_buffer, output_buffer_len, "  %2lu.%01lu%%  %2lu.%01lu%%",
		int_fp1.i, int_fp1.dec, int_fp2.i, int_fp2.dec);
	write(1, output_buffer, strlen(output_buffer));

/* cpu cycles/pingpong for ping
	if (config.set_affinity == true) {
		snprintf(output_buffer, output_buffer_len, ", ping: %d.%d cyc.",
			(int)i_stats->cpi[0], (int)(i_stats->cpi[0] * 100.0L) % 100);
		write(1, output_buffer, strlen(output_buffer));
		snprintf(output_buffer, output_buffer_len, ", %ld/%ld csw",
			i_stats->rusage[0].ru_nvcsw, i_stats->rusage[0].ru_nivcsw);
		write(1, output_buffer, strlen(output_buffer));
	}
*/

/* cpu cycles/pingpong for pong
	if (config.set_affinity == true) {

		snprintf(output_buffer, output_buffer_len, ", pong: %d.%d cyc.",
			(int)i_stats->cpi[1], (int)(i_stats->cpi[1] * 100.0L) & 100);
		write(1, output_buffer, strlen(output_buffer));
		snprintf(output_buffer, output_buffer_len, ", %ld csw",
			i_stats->csw[1]);
		write(1, output_buffer, strlen(output_buffer));
	}
*/

	write(1, "\n", 1);
}

void store_last_stats(struct interval_stats_struct *i_stats) {
	/* cleanup things for the next time we come back */
	memcpy((void *)&run_data->thread_stats[0].last_rusage,
		(void *)&run_data->thread_stats[0].rusage, sizeof(struct rusage));
	memcpy((void *)&run_data->thread_stats[1].last_rusage,
		(const void *)&run_data->thread_stats[1].rusage, sizeof(struct rusage));

	run_data->last_ping_count = i_stats->current_count;
	run_data->last_stats_time = i_stats->current_time;
	run_data->thread_stats[0].last_tsc = run_data->thread_stats[0].tsc;
	run_data->thread_stats[1].last_tsc = run_data->thread_stats[1].tsc;
}
