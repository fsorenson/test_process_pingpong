#include "race.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>

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

int cleanup_race() {

	return 0;
}

void __attribute__((constructor)) comm_add_race() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("race");
	init_info.help_text = strdup("both threads repeatedly write their own value--no pingpong");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_race_pair;
	ops.comm_do_ping = do_ping_race;
	ops.comm_do_pong = do_pong_race;
	ops.comm_cleanup = cleanup_race;

	comm_mode_do_initialization(&init_info, &ops);
	free(init_info.name);
	free(init_info.help_text);
}
ADD_COMM_MODE(race, comm_add_race);
