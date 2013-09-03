/*
	This file is part of test_process_pingpong

	Copyright 2013 Frank Sorenson (frank@tuxrocks.com)


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "stats_final.h"

#include "stats_common.h"
#include "units.h"
#include <string.h>
#include <stdio.h>

#define STAT_NAME_WIDTH         30
#define STAT_VALUE_WIDTH        15

const char *sched_line_strings[] = {
	"se.vruntime",
	"se.sum_exec_runtime",
	"se.statistics.wait_start",
	"se.statistics.sleep_start",
	"se.statistics.block_start",
	"se.statistics.sleep_max",
	"se.statistics.block_max",
	"se.statistics.exec_max",
	"se.statistics.slice_max",
	"se.statistics.wait_max",
	"se.statistics.wait_sum",
	"se.statistics.wait_count",
	"se.statistics.iowait_sum",
	"se.statistics.iowait_count",
/*
	"se.nr_migrations",
	"se.statistics.nr_migrations_cold",
	"se.statistics.nr_failed_migrations_affine",
	"se.statistics.nr_failed_migrations_running",
	"se.statistics.nr_failed_migrations_hot",
	"se.statistics.nr_forced_migrations",
	"se.statistics.nr_wakeups",
	"se.statistics.nr_wakeups_sync",
	"se.statistics.nr_wakeups_migrate",
	"se.statistics.nr_wakeups_local",
	"se.statistics.nr_wakeups_remote",
	"se.statistics.nr_wakeups_affine",
	"se.statistics.nr_wakeups_affine_attempts",
	"se.statistics.nr_wakeups_passive",
	"se.statistics.nr_wakeups_idle",
	"avg_atom",
	"avg_per_cpu",
*/
	"nr_switches",
	"nr_voluntary_switches",
	"nr_involuntary_switches",
	"se.load.weight"
};

struct sched_data_struct {
	char *key;
	char *value1;
	char *value2;
};

static int interesting_sched_stat(const char *check_str) {
	unsigned i;

	for (i = 0 ; i < sizeof(sched_line_strings)/sizeof(sched_line_strings[0]) ; i ++) {
		if (!strcmp(sched_line_strings[i], check_str))
			return 1;
	}
	return 0;
}

static int split_sched_lines(struct sched_data_struct *sched_data, char *buf1, char *buf2) {
	unsigned long count1a, count2a;
	unsigned long count1b, count2b;

	count1a = strcspn(buf1, ": ");
	count2a = strcspn(buf2, ": ");
	if ((count1a <= 0) || (count2a <= 0))
		return 0;
	buf1[count1a] = '\0';
	buf2[count2a] = '\0';
	if (strcmp(buf1, buf2))
		return 0; /* keys don't match */
	sched_data->key = buf1;

	count1a ++; count2a ++;

	count1b = strspn(&buf1[count1a], ": ");
	count2b = strspn(&buf2[count2a], ": ");

	if ((count1b == 0) || (count2b == 0)) {
		sched_data->key = sched_data->value1 = sched_data->value2 = 0;
		return 0;
	}

	sched_data->value1 = buf1 + count1a + count1b;
	sched_data->value2 = buf2 + count2a + count2b;

	return 1;
}

static int __PURE count_sched_lines(const char *buf) {
	int count = 0;
	size_t index = 0;

	while (buf[index] != '\0') {
		if (buf[index++] == '\n')
			count ++;
	}
	return count;
}

static void parse_sched_data(void) {
	char *sched_tok1, *sched_tok2;
	char *save_ptr1, *save_ptr2;
	int sched_lines;
	struct sched_data_struct *sched_data;
	int line_count = 0;
	int ret;

	sched_lines = count_sched_lines(run_data->thread_stats[0].sched_data);
	sched_data = calloc((size_t)sched_lines, sizeof(struct sched_data_struct));

	sched_tok1 = strtok_r(run_data->thread_stats[0].sched_data, "\n", &save_ptr1);
	sched_tok2 = strtok_r(run_data->thread_stats[1].sched_data, "\n", &save_ptr2);

	while((sched_tok1 != NULL) && (sched_tok2 != NULL)) {
		ret = split_sched_lines(&sched_data[line_count], sched_tok1, sched_tok2);
		if (ret) {
			if (sched_data[line_count].key != 0) {
				if (interesting_sched_stat(sched_data[line_count].key)) {
					printf("%-*s %-*s %-*s\n",
						STAT_NAME_WIDTH, sched_data[line_count].key,
						STAT_VALUE_WIDTH, sched_data[line_count].value1,
						STAT_VALUE_WIDTH, sched_data[line_count].value2);
				}
				line_count ++;
			}
		}
		sched_tok1 = strtok_r(NULL, "\n", &save_ptr1);
		sched_tok2 = strtok_r(NULL, "\n", &save_ptr2);
	}
	free(sched_data);
}

void output_final_stats(void) {
#define OUTPUT_BUFFER_LEN 400
	static char output_buffer[OUTPUT_BUFFER_LEN];
	size_t output_buffer_len = OUTPUT_BUFFER_LEN;
#undef OUTPUT_BUFFER_LEN
#define TEMP_STRING_LEN 30
	static char temp_string1[TEMP_STRING_LEN] = { 0 };
	static char temp_string2[TEMP_STRING_LEN] = { 0 };
	size_t temp_string_len = TEMP_STRING_LEN;
#undef TEMP_STRING_LEN
	static struct interval_stats_struct i_stats = { 0 };
	integer_fixed_point_t int_fp1, int_fp2;

	memset(&i_stats, 0, sizeof(struct interval_stats_struct));

	safe_write(config.output_fd, output_buffer, output_buffer_len, "\n");

	i_stats.current_count = run_data->ping_count;
	i_stats.current_time = get_time();

	i_stats.interval_time = i_stats.run_time = i_stats.current_time - run_data->start_time;
	safe_write(config.output_fd, output_buffer, output_buffer_len, "Elapsed time: %7s\n",
		subsec_string(temp_string1, i_stats.interval_time, 1));


	i_stats.interval_count = i_stats.current_count;
	safe_write(config.output_fd, output_buffer, output_buffer_len, "Ping/pong count: %llu\n", i_stats.interval_count);


	i_stats.iteration_time = i_stats.interval_time / i_stats.interval_count;
	safe_write(config.output_fd, output_buffer, output_buffer_len, "Ping/pong time: %s\n",
		subsec_string(temp_string2, i_stats.iteration_time, 2));


	safe_write(config.output_fd, output_buffer, output_buffer_len, "Pings/second: %llu\n",
		(unsigned long long)((long double)i_stats.interval_count / i_stats.interval_time));


	i_stats.rusage[0].ru_nvcsw = run_data->thread_stats[0].rusage.ru_nvcsw;
	i_stats.rusage[0].ru_nivcsw = run_data->thread_stats[0].rusage.ru_nivcsw;
	i_stats.rusage[1].ru_nvcsw = run_data->thread_stats[1].rusage.ru_nvcsw;
	i_stats.rusage[1].ru_nivcsw = run_data->thread_stats[1].rusage.ru_nivcsw;

	i_stats.csw[0] = i_stats.rusage[0].ru_nvcsw + i_stats.rusage[0].ru_nivcsw;
	i_stats.csw[1] = i_stats.rusage[1].ru_nvcsw + i_stats.rusage[1].ru_nivcsw;

	i_stats.rusage[0].ru_utime = run_data->thread_stats[0].last_rusage.ru_utime;
	i_stats.rusage[0].ru_stime = run_data->thread_stats[0].last_rusage.ru_stime;

	i_stats.rusage[1].ru_utime = run_data->thread_stats[1].last_rusage.ru_utime;
	i_stats.rusage[1].ru_stime = run_data->thread_stats[1].last_rusage.ru_stime;

	if (config.set_affinity == true) {
		i_stats.interval_tsc[0] = run_data->thread_stats[0].tsc - run_data->thread_stats[0].start_tsc;
		i_stats.interval_tsc[1] = run_data->thread_stats[1].tsc - run_data->thread_stats[1].start_tsc;
		i_stats.mhz[0] = i_stats.interval_tsc[0] / i_stats.interval_time / 1000 / 1000;
		i_stats.mhz[1] = i_stats.interval_tsc[1] / i_stats.interval_time / 1000 / 1000;

		i_stats.cpi[0] = (long double)i_stats.interval_tsc[0] / (long double)i_stats.interval_count;
		i_stats.cpi[1] = (long double)i_stats.interval_tsc[1] / (long double)i_stats.interval_count;
	} else {
		i_stats.interval_tsc[0] = i_stats.interval_tsc[1] = 0;
		i_stats.mhz[0] = i_stats.mhz[1] = 0;
		i_stats.cpi[0] = i_stats.cpi[1] = 0;
	}

        int_fp1 = f_to_fp(1, ((i_stats.rusage[0].ru_utime.tv_sec * 100.0L) +
		(i_stats.rusage[0].ru_utime.tv_usec / 1.0e4L) / i_stats.interval_time));


/*
				  ___________ PING ___________   ___________ PONG ___________
     time   cycles/sec  cycle time |   vol/inv csw   user    sys |   vol/inv csw   user    sys
*/
	write(1, "\n", 1);

	parse_sched_data();
}
