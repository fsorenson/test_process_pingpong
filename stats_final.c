#include "stats_final.h"

#include <string.h>
#include <stdio.h>

#define STAT_NAME_WIDTH         30
#define STAT_VALUE_WIDTH        15

const char *sched_line_strings[] = {
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
	char *p1, *p2;

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

	p1 = buf1 + count1a;
	p2 = buf2 + count2a;
	count1b = strspn(p1, ": ");
	count2b = strspn(p2, ": ");
	if ((count1b = 0) || (count2b = 0)) {
		printf("umm.  unable to parse something\n");
		sched_data->key = sched_data->value1 = sched_data->value2 = 0;
		return 0;
	}

	sched_data->value1 = buf1 + count1a + count1b;
	sched_data->value2 = buf2 + count2a + count2b;

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

	parse_sched_data();
}
