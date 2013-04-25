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
* one thread just does nothing...  in various ways
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/
inline int __PINGPONG_FN do_ping_nop1(int thread_num) {
	(void) thread_num;

	while (1) {
		run_data->ping_count ++;

		while (1 != 1);
		while (1 != 1);
	}
}
inline int __PINGPONG_FN do_ping_nop2(int thread_num) {
	(void) thread_num;

	while (1) {
		run_data->ping_count ++;

		*nop_var = 1;
		while (*nop_var != 1) {
		}
	}
}
inline int __PINGPONG_FN do_ping_nop3(int thread_num) {
	(void) thread_num;

	while (1) {
		run_data->ping_count ++;
	}
}

inline int __PINGPONG_FN do_pong_nop(int thread_num) {
	(void) thread_num;

	while (1) {
		nanosleep(&nop_ts, NULL);
	}
}
int __CONST cleanup_nop() {

	return 0;
}

void __attribute__((constructor)) comm_add_nop1() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("nop1");
	init_info.help_text = strdup("both threads do nothing, but one even sleeps while doing it");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_nop_pair;
	ops.comm_do_ping = do_ping_nop1;
	ops.comm_do_pong = do_pong_nop;
	ops.comm_cleanup = cleanup_nop;

	comm_mode_do_initialization(&init_info, &ops);
}
void __attribute__((constructor)) comm_add_nop2() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("nop2");
	init_info.help_text = strdup("first thread sets the variable, then tests it (other thread sleeps)");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_nop_pair;
	ops.comm_do_ping = do_ping_nop2;
	ops.comm_do_pong = do_pong_nop;
	ops.comm_cleanup = cleanup_nop;

	comm_mode_do_initialization(&init_info, &ops);
	free(init_info.name);
	free(init_info.help_text);
}
void __attribute__((constructor)) comm_add_nop3() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("nop3");
	init_info.help_text = strdup("literally do nothing...  just a loop");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_nop_pair;
	ops.comm_do_ping = do_ping_nop3;
	ops.comm_do_pong = do_pong_nop;
	ops.comm_cleanup = cleanup_nop;

	comm_mode_do_initialization(&init_info, &ops);
}

ADD_COMM_MODE(nop3, comm_add_nop3);
ADD_COMM_MODE(nop2, comm_add_nop2);
ADD_COMM_MODE(nop1, comm_add_nop1);
