#include "stats_final.h"

#include <string.h>

static int __PURE count_sched_lines(const char *buf) {
	int count = 0;
	int index = 0;
	char *p;

	while (buf + index != NULL) {
		count ++;
		p = strchr(buf + index, '\n');
		if (p != NULL)
			index ++;
	}
	return count;
}

static void parse_sched_data(void) {
	int sched_lines;

	sched_lines = count_sched_lines(run_data->thread_stats[0].sched_data);
}

void output_final_stats(void) {

	parse_sched_data();
}
