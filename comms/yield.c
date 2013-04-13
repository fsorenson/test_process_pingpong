#include "yield.h"
#include "comms.h"

#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>


struct timespec yield_ts;

int make_yield_pair(int fd[2]) {
	static int yield_num = 0;

	if (yield_num == 0) {
		yield_ts.tv_sec = 0;
		yield_ts.tv_nsec = 1000000;
	}

	fd[0] = fd[1] = yield_num;

	yield_num ++;
	return 0;
}

inline int do_send_yield(int fd) {
	if (fd == 0) {
		sched_yield();
		return 1;
	} else {
		 nanosleep(&yield_ts, NULL);
		 return 0;
	}

	return 1;
}
inline int do_recv_yield(int fd) {
	(void) fd;

	return 1;
}

void __attribute__((constructor)) comm_add_yield() {
	struct comm_mode_ops_struct ops;

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_yield_pair;
	ops.comm_do_send = do_send_yield;
	ops.comm_do_recv = do_recv_yield;

	comm_mode_do_initialization("yield", &ops);

}
ADD_COMM_MODE(yield, comm_add_yield);
