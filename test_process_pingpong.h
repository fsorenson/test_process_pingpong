#ifndef __TEST_PROCESS_PINGPONG_H__
#define __TEST_PROCESS_PINGPONG_H__

/*
        test_process_pingpong: program to test 'ping-pong' performance
		between two processes on a host

        by Frank Sorenson (frank@tuxrocks.com) 2013
*/

#include "common.h"
#include "comms.h"

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
#define DEFAULT_STATS_INTERVAL	5 /* output the stats every # seconds */
#define DEFAULT_STATS_SUMMARY	true /* display a summary immediately prior to exit */



typedef enum {
	thread_mode_fork, thread_mode_thread, thread_mode_pthread, thread_mode_context
} __PACKED thread_modes;




/* this doesn't need to be modified once it's set up, so doesn't need to be shared */
struct config_struct {
	char verbosity;
	thread_modes thread_mode;

	bool stats_summary; /* summary prior to exit */
	bool set_affinity;

//	char dummy1;
	short num_cpus;
	short num_online_cpus;
	short cpu[2]; /* cpu affinity settings */

	int comm_mode_index;
//	char dummy2[2];

	uid_t uid;
	gid_t gid;
	uid_t euid;
	gid_t egid;


	int *pair1;
	int *pair2;

	int pairs[2][2];
	int mouth[2];
	int ear[2];

	int min_stack;
	int cur_sched_policy;
	int cur_sched_prio;


	int sched_policy;
	int sched_prio;

	char dummy5[4];

	long stats_interval; /* in seconds */
	unsigned long max_execution_time; /* in seconds */

	char *argv0;

	char *sched_string;

	double sched_rr_quantum;

	void *stack[2];

	COMM_MODE_OPS;
/*
	int (*comm_init)();
	int (*comm_make_pair)(int fd[2]);
	int (*comm_pre)();
	int (*comm_do_send)(int s);
	int (*comm_do_recv)(int s);
	int (*comm_interrupt)();
	int (*comm_cleanup)();
*/
};

extern struct config_struct config;

struct thread_info_struct {
	//pthread_t thread_id;
	int thread_id;
	int thread_num;
	int pid;
	int tid;
	char *thread_name;
};

struct thread_stats_struct {
	unsigned long long start_tsc;
	unsigned long long tsc;
	unsigned long long last_tsc;
	struct rusage rusage;
	struct rusage last_rusage;
//	unsigned int thread_id;
};

extern struct thread_info_struct *thread_info;

struct run_data_struct {
	unsigned long long volatile ping_count;  /* 8 bytes !?!? */
#pragma pack(1)
	volatile bool stop; /* stop request to the threads */

	/*
	 * monitor sets to 'true' prior to raising request_rusage flags
	 * monitor knows both threads have provided the info
	 * if ((rusage_req_in_progress == true) && (rusage_req[0] == false) && (rusage_req[1] == false))
	 *
	 */
	volatile bool rusage_req_in_progress;

	/*
	 * set to 'true' to request the rusage stats from the threads
	 * threads will provide stats to 'rusage' below, then
	 * set their flag back to 'false'
	 */
	volatile bool rusage_req[2]; /* rusage request */
	char dummy1[4];
	char dummy2[8];
#pragma pack()
	unsigned long long last_ping_count;

	long double start_time;
	long double timeout_time; /* should stop by this time...  set to start_time + execution time */

	long double last_stats_time;


	long double rusage_time; /* time of most recent rusage report */
	long double last_rusage_time; /* time of last report */


	struct thread_info_struct thread_info[2];
	struct thread_stats_struct thread_stats[2];
//	char really_big_dummy[10240];

};


extern volatile struct run_data_struct *run_data;




#endif
