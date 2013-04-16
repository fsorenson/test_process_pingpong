#include "nop.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <time.h>

int *nop_var;

struct timespec nop_ts;

int make_nop_pair(int fd[2]) {
	static int nop_num = 0;

	if (nop_num == 0) {
		nop_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*nop_var = 0;

		nop_ts.tv_sec = 0;
		nop_ts.tv_nsec = 1000000;
	}

	fd[0] = nop_num;
	fd[1] = nop_num;

	nop_num ++;
	return 0;
}


/*
* one thread just does nothing...
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/
inline int __PINGPONG_FN do_ping_nop(int thread_num) {
	(void) thread_num;

	while (1) {
		run_data->ping_count ++;

		do {
			*nop_var ^= 1;
		} while (0);
		do {

		} while (0);
	}
}
inline int __PINGPONG_FN do_pong_nop(int thread_num) {
	(void) thread_num;

	while (1) {
		nanosleep(&nop_ts, NULL);
	}
}
/*
inline int do_send_nop(int fd) {
	if (fd == 0) {
		*nop_var ^= 1;

		return 1;
	} else {
		nanosleep(&nop_ts, NULL);
		return 0;
	}
}
inline int do_recv_nop(int fd) {
	(void)fd;

	return 1;
}
*/
int cleanup_nop() {

	return 0;
}

void __attribute__((constructor)) comm_add_nop() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = "nop";

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_nop_pair;
	ops.comm_do_ping = do_ping_nop;
	ops.comm_do_pong = do_pong_nop;
//	ops.comm_do_send = do_send_nop;
//	ops.comm_do_recv = do_recv_nop;
	ops.comm_cleanup = cleanup_nop;

	comm_mode_do_initialization(&init_info, &ops);
}

ADD_COMM_MODE(nop, comm_add_nop);
