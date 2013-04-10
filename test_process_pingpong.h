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
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef SYS_eventfd
  #define HAVE_EVENTFD
#endif

#ifdef SYS_getcpu
  #define HAVE_GETCPU
#endif


// 64 KB stack
#define STACK_SIZE	1024*64

#define DEFAULT_SCHED		SCHED_FIFO
#define DEFAULT_SCHED_PRIO	1

#define DEFAULT_COMM_MODE	comm_mode_tcp
#define DEFAULT_THREAD_MODE	thread_mode_thread
#define DEFAULT_EXECUTION_TIME	30 /* max time to run the test (seconds) */
#define DEFAULT_STATS_INTERVAL	1 /* output the stats every # seconds */
#define DEFAULT_STATS_SUMMARY	true /* display a summary immediately prior to exit */

#define __PACKED __attribute__ ((packed))


typedef enum __PACKED { no = 0, false = 0, yes = 1, true = 1 } bool;


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

/* this doesn't need to be modified once it's set up, so doesn't need to be shared */
struct config_struct {
	int verbosity;
	char *argv0;
	int num_cpus;
	int num_online_cpus;
	int min_stack;

	uid_t uid;
	gid_t gid;
	uid_t euid;
	gid_t egid;

	union {
		comm_modes comm_mode;
		int comm_mode_i;
	};
	thread_modes thread_mode;

	int cur_sched_policy;
	int cur_sched_prio;
	double sched_rr_quantum;

	char *sched_string;
	int sched_policy;
	int sched_prio;


	unsigned long max_execution_time; /* in seconds */
	unsigned long stats_interval; /* in seconds */
	bool stats_summary; /* summary prior to exit */

	int cpu[2];

	int pairs[2][2];
	int *pair1;
	int *pair2;
	int mouth[2];
	int ear[2];

	void *stack[2];

	int (*setup_comm)();
	int (*make_pair)(int fd[2]);
	int (*do_send)(int s);
	int (*do_recv)(int s);
	int (*cleanup)();
};

extern struct config_struct config;

struct run_data_struct {
//#pragma pack(1)
	unsigned long long volatile ping_count;  /* 8 bytes !?!? */
	volatile bool stop:8; /* stop request to the threads */

	/*
	 * monitor sets to 'true' prior to raising request_rusage flags
	 * monitor knows both threads have provided the info
	 * if ((rusage_req_in_progress == true) && (rusage_req[0] == false) && (rusage_req[1] == false))
	 *
	 */
	volatile bool rusage_req_in_progress:8;

	/*
	 * set to 'true' to request the rusage stats from the threads
	 * threads will provide stats to 'rusage' below, then
	 * set their flag back to 'false'
	 */
	volatile bool rusage_req[2]; /* rusage request */
//#pragma pack()

	pid_t ping_pid;
	pid_t pong_pid;

	pthread_t ping_thread;
	pthread_t pong_thread;

	struct rusage rusage[2]; /* rusage stats return location */
	long double rusage_time; /* time of most recent rusage report */
	struct rusage last_rusage[2]; /* values at last report */
	long double last_rusage_time; /* time of last report */

	long double start_time;
	long double timeout_time; /* should stop by this time...  set to start_time + execution time */
};


extern volatile struct run_data_struct *run_data;




#endif
