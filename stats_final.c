#include "stats_final.h"

#include <string.h>

struct line_data_struct {
	char *key;
	char *value1;
	char *value2;
};

static int split_sched_lines(struct line_data_struct *line_data, char *buf1, char *buf2) {

	return 1;
}

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
	struct line_data_struct *line_data;
	int line_count = 0;
	int ret;

	sched_lines = count_sched_lines(run_data->thread_stats[0].sched_data);
	line_data = calloc((size_t)sched_lines, sizeof(struct line_data_struct));

	sched_tok1 = strtok_r(run_data->thread_stats[0].sched_data, "\n", &save_ptr1);
	sched_tok2 = strtok_r(run_data->thread_stats[1].sched_data, "\n", &save_ptr2);

	while((sched_tok1 != NULL) && (sched_tok2 != NULL)) {
		ret = split_sched_lines(&line_data[line_count], sched_tok1, sched_tok2);

		sched_tok1 = strtok_r(NULL, "\n", &save_ptr1);
		sched_tok2 = strtok_r(NULL, "\n", &save_ptr2);
	}
	free(line_data);
}

void output_final_stats(void) {

	parse_sched_data();
}
