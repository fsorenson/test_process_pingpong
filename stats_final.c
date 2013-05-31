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
	char *sched_tok1, *sched_tok2;
	char *save_ptr1, *save_ptr2;
	int sched_lines;

	sched_lines = count_sched_lines(run_data->thread_stats[0].sched_data);

	sched_tok1 = strtok_r(run_data->thread_stats[0].sched_data, "\n", &save_ptr1);
	sched_tok2 = strtok_r(run_data->thread_stats[1].sched_data, "\n", &save_ptr2);

	while((sched_tok1 != NULL) && (sched_tok2 != NULL)) {
		sched_tok1 = strtok_r(NULL, "\n", &save_ptr1);
		sched_tok2 = strtok_r(NULL, "\n", &save_ptr2);
	}
}

void output_final_stats(void) {

	parse_sched_data();
}
