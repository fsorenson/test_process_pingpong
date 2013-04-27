#include "race.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>

char comm_name_race[] = "race";
char comm_help_text_race[] = "both threads repeatedly write their own value--no pingpong";

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


inline int __PINGPONG_FN do_ping_race(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		*race_var = 1;
	}
}

inline int __PINGPONG_FN do_pong_race(int thread_num) {
	(void)thread_num;
	while (1) {
		*race_var = 0;
	}
}

int __CONST cleanup_race() {

	return 0;
}

static struct comm_mode_init_info_struct comm_info_race = {
	.name = comm_name_race,
	.help_text = comm_help_text_race
};

static struct comm_mode_ops_struct comm_ops_race = {
	.comm_make_pair = make_race_pair,
	.comm_do_ping = do_ping_race,
	.comm_do_pong = do_pong_race,
	.comm_cleanup = cleanup_race
};

void __attribute__((constructor)) comm_add_race() {
	comm_mode_do_initialization(&comm_info_race, &comm_ops_race);
}
ADD_COMM_MODE(race, comm_add_race);
