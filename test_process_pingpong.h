#ifndef __TEST_PROCESS_PINGPONG_H__
#define __TEST_PROCESS_PINGPONG_H__

/*
        test_process_pingpong: program to test 'ping-pong' performance
		between two processes on a host

        by Frank Sorenson (frank@tuxrocks.com) 2013
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_EVENTFD
#include "eventfd.h"
#endif

// 64 KB stack
#define STACK_SIZE	1024*64

#define DEFAULT_SCHED		SCHED_FIFO
#define DEFAULT_SCHED_PRIO	1

typedef enum { no = 0, false = 0, yes = 1, true = 1 } bool;


typedef enum { comm_mode_tcp = 0, comm_mode_udp = 1, comm_mode_pipe = 2, comm_mode_sockpair = 3
#ifdef HAVE_EVENTFD
	, comm_mode_eventfd = 4
#endif
	, comm_mode_sem = 5, comm_mode_busy_sem = 6
	, comm_mode_futex = 7
	, comm_mode_spin = 8
	, comm_mode_nop = 9
	, comm_mode_mq = 10
} comm_modes;

typedef enum { thread_mode_fork, thread_mode_thread, thread_mode_pthread, thread_mode_context } thread_modes;

struct config_struct {
	char *argv0;
	int num_cpus;
	int num_online_cpus;

	uid_t uid;
	gid_t gid;
	uid_t euid;
	gid_t egid;

	union {
		comm_modes comm_mode;
		int comm_mode_i;
	};
	thread_modes thread_mode;
#ifdef SET_PRIORITIES
	int cur_sched_policy;
	int cur_sched_prio;
	double sched_rr_quantum;

	char *sched_string;
	int sched_policy;
	int sched_prio;
#endif

	unsigned long max_execution_time; /* in seconds */

	int cpu1;
	int cpu2;

	int pairs[2][2];
	int *pair1;
	int *pair2;

	int parent_mouth;
	int parent_ear;
	int child_mouth;
	int child_ear;

	int (*setup_comm)();
	int (*make_pair)(int fd[2]);
	int (*do_send)(int s);
	int (*do_recv)(int s);
	int (*cleanup)();
};

extern struct config_struct config;

struct stats_struct {
	volatile bool stop;
	pid_t ping_pid;
	pid_t pong_pid;

	long double start_time;
	long double timeout_time; /* should stop by this time...  set to start_time + execution time */

	unsigned long long volatile ping_count;

};

extern volatile struct stats_struct *stats;

#endif
