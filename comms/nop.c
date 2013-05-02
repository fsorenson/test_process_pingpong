#include "nop.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <time.h>

char comm_name_nop1[] = "nop1";
char comm_help_text_nop1[] = "first thread only sets the variable, then tests it (second thread sleeps)";
char comm_name_nop2[] = "nop2";
char comm_help_text_nop2[] = "first thread only sets the variable (second thread sleeps)";
char comm_name_nop3[] = "nop3";
char comm_help_text_nop3[] = "first thread only tests the variable (second thread sleeps)";
char comm_name_nop4[] = "nop4";
char comm_help_text_nop4[] = "both threads literally do nothing, but one even sleeps while doing it";

volatile int *nop_var;

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

	*nop_var = 1;
	while (1) {
		run_data->ping_count ++;

		*nop_var = *nop_var;
		__asm__ __volatile__("");
		do { } while (*nop_var != *nop_var);
		__asm__ __volatile__("");
	}
}
inline int __PINGPONG_FN do_ping_nop2(int thread_num) {
	(void) thread_num;

	*nop_var = 1;
	while (1) {
		run_data->ping_count ++;

		do {} while (unlikely(*nop_var != 1));
		__asm__ __volatile__("");
	}
}
inline int __PINGPONG_FN do_ping_nop3(int thread_num) {
	(void) thread_num;

	*nop_var = 1;
	while (1) {
		run_data->ping_count ++;

		*nop_var = 1;
		__asm__ __volatile__("");
	}
}
inline int __PINGPONG_FN do_ping_nop4(int thread_num) {
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
int __CONST cleanup_nop(void) {

	return 0;
}

static struct comm_mode_init_info_struct comm_info_nop1 = {
	.name = comm_name_nop1,
	.help_text = comm_help_text_nop1
};

static struct comm_mode_ops_struct comm_ops_nop1 = {
	.comm_make_pair = make_nop_pair,
	.comm_do_ping = do_ping_nop1,
	.comm_do_pong = do_pong_nop,
	.comm_cleanup = cleanup_nop
};

static struct comm_mode_init_info_struct comm_info_nop2 = {
	.name = comm_name_nop2,
	.help_text = comm_help_text_nop2
};

static struct comm_mode_ops_struct comm_ops_nop2 = {
	.comm_make_pair = make_nop_pair,
	.comm_do_ping = do_ping_nop2,
	.comm_do_pong = do_pong_nop,
	.comm_cleanup = cleanup_nop
};

static struct comm_mode_init_info_struct comm_info_nop3 = {
	.name = comm_name_nop3,
	.help_text = comm_help_text_nop3
};

static struct comm_mode_ops_struct comm_ops_nop3 = {
	.comm_make_pair = make_nop_pair,
	.comm_do_ping = do_ping_nop3,
	.comm_do_pong = do_pong_nop,
	.comm_cleanup = cleanup_nop
};

static struct comm_mode_init_info_struct comm_info_nop4 = {
	.name = comm_name_nop4,
	.help_text = comm_help_text_nop4
};

static struct comm_mode_ops_struct comm_ops_nop4 = {
	.comm_make_pair = make_nop_pair,
	.comm_do_ping = do_ping_nop4,
	.comm_do_pong = do_pong_nop,
	.comm_cleanup = cleanup_nop
};

void comm_add_nop1(void) {
	comm_mode_do_initialization(&comm_info_nop1, &comm_ops_nop1);
}
void comm_add_nop2(void) {
	comm_mode_do_initialization(&comm_info_nop2, &comm_ops_nop2);
}
void comm_add_nop3(void) {
	comm_mode_do_initialization(&comm_info_nop3, &comm_ops_nop3);
}
void comm_add_nop4(void) {
	comm_mode_do_initialization(&comm_info_nop4, &comm_ops_nop4);
}

NEW_ADD_COMM_MODE(nop4, "both threads literally do nothing, but one even sleeps while doing it", &comm_ops_nop4);
NEW_ADD_COMM_MODE(nop3, "first thread only tests the variable (second thread sleeps)", &comm_ops_nop3);
NEW_ADD_COMM_MODE(nop2, "first thread only sets the variable (second thread sleeps", &comm_ops_nop2);
NEW_ADD_COMM_MODE(nop1, "first thread only sets the variable, then tests it (second thread sleeps)", &comm_ops_nop1);
