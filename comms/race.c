#include "race.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>

char comm_name_race1[] = "race1";
char comm_help_text_race1[] = "both threads repeatedly write their own value--no pingpong";
char comm_name_race2[] = "race2";
char comm_help_text_race2[] = "both threads repeatedly write their own value _AND_ increment the counter--no pingpong";

int volatile *race_var;

int make_race_pair(int fd[2]) {
	static int race_num = 0;

	if (race_num == 0) {
		race_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*race_var = 0;
	}

	fd[0] = race_num;
	fd[1] = race_num;

	race_num ++;
	return 0;
}


inline int __PINGPONG_FN do_ping_race1(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		*race_var = 1;
	}
}

inline int __PINGPONG_FN do_ping_race2(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		*race_var = 1;
	}
}

inline int __PINGPONG_FN do_pong_race1(int thread_num) {
	(void)thread_num;
	while (1) {
		*race_var = 0;
	}
}

inline int __PINGPONG_FN do_pong_race2(int thread_num) {
	(void)thread_num;
	while (1) {
		run_data->ping_count ++;

		*race_var = 0;
	}
}

int __CONST cleanup_race() {

	return 0;
}

static struct comm_mode_init_info_struct comm_info_race1 = {
	.name = comm_name_race1,
	.help_text = comm_help_text_race1
};

static struct comm_mode_ops_struct comm_ops_race1 = {
	.comm_make_pair = make_race_pair,
	.comm_do_ping = do_ping_race1,
	.comm_do_pong = do_pong_race1,
	.comm_cleanup = cleanup_race
};
static struct comm_mode_init_info_struct comm_info_race2 = {
	.name = comm_name_race2,
	.help_text = comm_help_text_race2
};

static struct comm_mode_ops_struct comm_ops_race2 = {
	.comm_make_pair = make_race_pair,
	.comm_do_ping = do_ping_race2,
	.comm_do_pong = do_pong_race2,
	.comm_cleanup = cleanup_race
};

void __attribute__((constructor)) comm_add_race1() {
	comm_mode_do_initialization(&comm_info_race1, &comm_ops_race1);
}
void __attribute__((constructor)) comm_add_race2() {
	comm_mode_do_initialization(&comm_info_race2, &comm_ops_race2);
}
ADD_COMM_MODE(race2, comm_add_race2);
ADD_COMM_MODE(race1, comm_add_race1);
