#include "unconstrained.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <time.h>


int *unconstrained_var;

struct timespec unconstrained_ts;

int make_unconstrained_pair(int fd[2]) {
	static int unconstrained_num = 0;

	if (unconstrained_num == 0) {
		unconstrained_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*unconstrained_var = 0;

		unconstrained_ts.tv_sec = 0;
		unconstrained_ts.tv_nsec = 1000000;
	}

	fd[0] = fd[1] = unconstrained_num;

	unconstrained_num ++;
	return 0;
}

inline int __PINGPONG_FN do_ping_unconstrained(int thread_num) {
	(void)thread_num;
	while (1) {
		run_data->ping_count ++;

		while (1 != 1);
		while (1 != 1);
	}
}

inline int __PINGPONG_FN  do_pong_unconstrained(int thread_num) {
	(void)thread_num;
	while (1) {
		nanosleep(&unconstrained_ts, NULL);
	}
}

inline int do_send_unconstrained(int fd) {
	if (fd == 0) {
//		*unconstrained_var ^= 1;

		return 1;
	} else {
		nanosleep(&unconstrained_ts, NULL);
		return 0;
	}
}
inline int do_recv_unconstrained(int fd) {
	(void)fd;

	return 1;
}

void __attribute__((constructor)) comm_add_unconstrained() {
	struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = "unconstrained";

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_unconstrained_pair;
	ops.comm_do_ping = do_ping_unconstrained;
	ops.comm_do_pong = do_pong_unconstrained;
	ops.comm_do_send = do_send_unconstrained;
	ops.comm_do_recv = do_recv_unconstrained;

	comm_mode_do_initialization(&init_info, &ops);
}

ADD_COMM_MODE(unconstrained, comm_add_unconstrained);
