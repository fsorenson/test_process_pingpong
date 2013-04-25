#include "yield.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>


static int volatile *yield_var;

struct timespec yield_ts;
static bool nop = false;

int make_yield_pair(int fd[2]) {
	static int yield_num = 0;

	if (yield_num == 0) {
		if (!strncmp(get_comm_mode_name(config.comm_mode_index), "yield_nop", 9))
			nop = true;

		if (nop == false) {
			yield_var = mmap(NULL, sizeof(int),
				PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
			*yield_var = 0;
		}

		yield_ts.tv_sec = 0;
		yield_ts.tv_nsec = 1000000;
	}

	fd[0] = fd[1] = yield_num;

	yield_num ++;
	return 0;
}

inline int __PINGPONG_FN do_ping_yield(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		*yield_var = 1;
		sched_yield();

		while (*yield_var != 0)
			sched_yield();
	}
}

inline int __PINGPONG_FN do_pong_yield(int thread_num) {
	(void)thread_num;

	while (1) {
		while (*yield_var != 1)
			sched_yield();

		*yield_var = 0;
		sched_yield();
	}
}


inline int __PINGPONG_FN do_ping_yield_nop(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;
		sched_yield();
	}
}

inline int __PINGPONG_FN do_pong_yield_nop(int thread_num) {
	(void)thread_num;

	while (1) {
		nanosleep(&yield_ts, NULL);
	}
}

void __attribute__((constructor)) comm_add_yield() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("yield");
	init_info.help_text = strdup("each thread calls sched_yield() until their turn to pingpong");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_yield_pair;
	ops.comm_do_ping = do_ping_yield;
	ops.comm_do_pong = do_pong_yield;

	comm_mode_do_initialization(&init_info, &ops);
	free(init_info.name);
	free(init_info.help_text);
}
void __attribute__((constructor)) comm_add_yield_nop() {
	struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("yield_nop");
	init_info.help_text = strdup("only tests rate at which sched_yield() re-schedules--no pingpong");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_yield_pair;
	ops.comm_do_ping = do_ping_yield_nop;
	ops.comm_do_pong = do_pong_yield_nop;

	comm_mode_do_initialization(&init_info, &ops);
	free(init_info.name);
	free(init_info.help_text);
}
ADD_COMM_MODE(yield_nop, comm_add_yield_nop);
ADD_COMM_MODE(yield, comm_add_yield);
