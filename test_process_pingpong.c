/*
        test_process_pingpong: program to test 'ping-pong' performance
		between two processes on a host

        by Frank Sorenson (frank@tuxrocks.com) 2013
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "units.h"
#include "sched.h"

#include "tcp.h"
#include "udp.h"
#include "socket_pair.h"
#include "sem.h"
#include "futex.h"
#include "spin.h"
#include "nop.h"
#include "mq.h"

#ifdef HAVE_EVENTFD
#include "eventfd.h"
#endif

struct config_struct config;
volatile struct stats_struct *stats;


//#define SET_PRIORITIES
struct comm_mode_info_struct {
	comm_modes comm_mode;
	char *name;

	int (*setup_comm)();
	int (*make_pair)(int fd[2]);
	int (*do_send)(int s);
	int (*do_recv)(int s);
	int (*cleanup)();
};

struct comm_mode_info_struct comm_mode_info[] = {
	{ comm_mode_tcp, "tcp", NULL, &make_tcp_pair, NULL, NULL, NULL }
	, { comm_mode_tcp, "t", NULL, &make_tcp_pair, NULL, NULL, NULL }
	, { comm_mode_udp, "udp", NULL, &make_udp_pair, NULL, NULL, NULL }
	, { comm_mode_udp, "u", NULL, &make_udp_pair, NULL, NULL, NULL }
	, { comm_mode_pipe, "pipe", NULL, &pipe, NULL, NULL, NULL }
	, { comm_mode_pipe, "p", NULL, &pipe, NULL, NULL, NULL }
	, { comm_mode_sockpair, "sockpair", NULL, &make_socket_pair, NULL, NULL, NULL }
	, { comm_mode_sockpair, "s", NULL, &make_socket_pair, NULL, NULL, NULL }
	, { comm_mode_eventfd, "eventfd", NULL, &make_eventfd_pair, &do_send_eventfd, &do_recv_eventfd, NULL }
	, { comm_mode_eventfd, "e", NULL, &make_eventfd_pair, &do_send_eventfd, &do_recv_eventfd, NULL }
	, { comm_mode_sem, "sem", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_sem, "sema", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_sem, "semaphore", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_busy_sem, "busy", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_busy_sem, "busysem", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_busy_sem, "busy_sem", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_futex, "futex", NULL, &make_futex_pair, do_send_futex, do_recv_futex, NULL }
	, { comm_mode_futex, "f", NULL, &make_futex_pair, do_send_futex, do_recv_futex, NULL }
	, { comm_mode_futex, "spin", NULL, &make_spin_pair, do_send_spin, do_recv_spin, NULL }
	, { comm_mode_futex, "nop", NULL, &make_nop_pair, &do_send_nop, &do_recv_nop, NULL }
	, { comm_mode_futex, "mq", NULL, &make_mq_pair, &do_send_mq, &do_recv_mq, &cleanup_mq }
};


/*
typedef enum { thread_mode_fork, thread_mode_thread, thread_mode_pthread, thread_mode_context } thread_modes;
*/
const char * thread_mode_strings[] = {
	[ thread_mode_fork ] = "fork"
	, [ thread_mode_thread ] = "threads"
	, [ thread_mode_pthread ] = "pthread"
};


int no_setup() {
	return 0;
}
int do_send(int fd);
int do_recv(int fd);
int no_cleanup() {
	return 0;
};


long double get_time() {
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return (ts.tv_sec * 1.0) + (ts.tv_nsec / 1000000000.0);
}

int usage() {
	printf("Usage: %s [options] [ <cpu #> <cpu #> ]\n", config.argv0);
	printf("\n");
	printf("Options:\n");
	printf("\t-m, --mode=MODE\n");
	printf("\t\tcommunication modes: tcp, udp, pipe, sockpair"
#ifdef HAVE_EVENTFD
	", eventfd"
#endif
	",\n\t\t\tsem | semaphore"
	",\n\t\t\tbusy | busysem | busy_sem | busy_semaphore"
	"\n\t\t\tfutex"
	"\n\t\t\tmq"
	"\n\t\t\tspin (be very careful)"
	"\n\t\t\tnop (no synchronization...one thread just sched_yields(), the other sleeps"
	"\n");

	printf("\n");
	printf("\t-t, --thread=MODE, --thread_mode=MODE\n");
	printf("\t\tthread modes: fork, clone, pthread\n");
	printf("\n");
	printf("\t-p, --priority=POLICY:PRIORITY\n");
	printf("\t\tpolicies: fifo, rr\n");
	printf("\t\tpriorities: 1-99 (careful with high priorities)\n");
	printf("\n");
	printf("\t<cpu #> - pin the threads to the designated CPUs\n");
	printf("\n");

	return 0;
}

int parse_thread_mode(char *arg) {
	if (! strcmp(arg, "fork") )
		config.thread_mode = thread_mode_fork;
	else if (! strcmp(arg, "pthread") )
		config.thread_mode = thread_mode_pthread;
	else if (! strcmp(arg, "clone") )
		config.thread_mode = thread_mode_thread;
	else if (! strcmp(arg, "thread") )
		config.thread_mode = thread_mode_thread;
	else {
		printf("Unknown threading mode: '%s'\n", arg);
	}
	printf("thread mode requested was '%s'\n", arg);
	printf("Using thread mode: %s\n", thread_mode_strings[config.thread_mode]);

	return 0;
}

char *get_comm_mode_name(int mode) {
	int i;

	for (i = 0 ; i < sizeof(comm_mode_info) / sizeof(struct comm_mode_info_struct) ; i ++) {
		if (mode == comm_mode_info[i].comm_mode)
			return comm_mode_info[i].name;
	}
	return "UNKNOWN";
}

int parse_comm_mode(char *arg) {
	int i;
	int found = -1;

	for (i = 0 ; i < sizeof(comm_mode_info) / sizeof(struct comm_mode_info_struct) ; i ++) {
		if (! strcmp(arg, comm_mode_info[i].name)) {
			found = i;
			break;
		}
	}
	if (!found) {
		printf("Unknown communication mode '%s'.  Falling back to default\n", arg);
		return -1;
	}

	config.comm_mode = comm_mode_info[found].comm_mode;
	config.setup_comm = &no_setup;
	config.make_pair = comm_mode_info[found].make_pair;
	config.do_send = comm_mode_info[found].do_send != NULL ? comm_mode_info[found].do_send : &do_send;
	config.do_recv = comm_mode_info[found].do_recv != NULL ? comm_mode_info[found].do_recv : &do_recv;
	config.cleanup = comm_mode_info[found].cleanup != NULL ? comm_mode_info[found].cleanup : &no_cleanup;

	return 0;
}


/*
int setup_comm_mode() {
	printf("Communicating via '%s'\n", get_comm_mode_name(config.comm_mode);
	switch (config.comm_mode) {
		case comm_mode_tcp: config.make_pair = &make_tcp_pair; break;
		case comm_mode_udp: config.make_pair = &make_udp_pair; break;
		case comm_mode_pipe: config.make_pair = &pipe; break;
		case comm_mode_sockpair: config.make_pair = &make_socket_pair; break;
#ifdef HAVE_EVENTFD
		case comm_mode_eventfd:
			config.make_pair = &make_eventfd_pair;
			config.do_send = &do_send_eventfd;
			config.do_recv = &do_recv_eventfd;
			break;
#endif
		case comm_mode_sem:
		case comm_mode_busy_sem:
			config.make_pair = &make_sem_pair;
			config.do_send = &do_send_sem;
			config.do_recv = &do_recv_sem;
			config.cleanup = &cleanup_sem;
			break;
		case comm_mode_futex:
			config.make_pair = &make_futex_pair;
			config.do_send = &do_send_futex;
			config.do_recv = &do_recv_futex;
			break;
		case comm_mode_spin:
			config.make_pair = &make_spin_pair;
			config.do_send = &do_send_spin;
			config.do_recv = &do_recv_spin;
			config.cleanup = &cleanup_spin;
			break;
		case comm_mode_nop:
			config.make_pair = &make_nop_pair;
			config.do_send = &do_send_nop;
			config.do_recv = &do_recv_nop;
			break;
		case comm_mode_mq:
			config.make_pair = &make_mq_pair;
			config.do_send = &do_send_mq;
			config.do_recv = &do_recv_mq;
			config.cleanup = &cleanup_mq;
			break;
	}

	return 0;
}
*/

int parse_opts(int argc, char *argv[]) {
	int opt = 0, long_index = 0;

	/* default settings */
	config.comm_mode = comm_mode_tcp;
	config.do_send = &do_send;
	config.do_recv = &do_recv;
	config.cleanup = &no_cleanup;
#ifdef SET_PRIORITIES
	config.sched_policy = DEFAULT_SCHED;
	config.sched_prio = DEFAULT_SCHED_PRIO;
#endif

	config.uid = getuid();
	config.gid = getgid();
	config.euid = geteuid();
	config.egid = getegid();
	config.num_cpus = num_cpus();
	config.num_online_cpus = num_online_cpus();

	config.max_execution_time = 30;
	config.thread_mode = thread_mode_fork;

	static struct option long_options[] = {
		{	"mode",		required_argument,	0,	'm'	}, /* communication mode */
		{	"tcp",		no_argument,		&config.comm_mode_i, comm_mode_tcp },
		{	"udp",		no_argument,		&config.comm_mode_i, comm_mode_udp },
		{	"pipe",		no_argument,		&config.comm_mode_i, comm_mode_pipe },
		{	"sockpair",	no_argument,		&config.comm_mode_i, comm_mode_sockpair },
		{	"eventfd",	no_argument,		&config.comm_mode_i, comm_mode_eventfd },
		{	"sem",		no_argument,		&config.comm_mode_i, comm_mode_sem },
		{	"sema",		no_argument,		&config.comm_mode_i, comm_mode_sem },
		{	"semaphore",	no_argument,		&config.comm_mode_i, comm_mode_sem },
		{	"busy_sem",	no_argument,		&config.comm_mode_i, comm_mode_busy_sem },
		{	"busy",		no_argument,		&config.comm_mode_i, comm_mode_busy_sem },

		{	"thread",	required_argument,	0,	't'	},
		{	"thread_mode",	required_argument,	0,	't'	},
#ifdef SET_PRIORITIES
		{	"sched",	required_argument,	0,	'p'	},
#endif
		{	0,		0,			0,	0	}
	};

	config.cpu1 = -1;
	config.cpu2 = -1;
	config.pair1 = config.pairs[0];
	config.pair2 = config.pairs[1];

	opterr = 0;
	while ((opt = getopt_long(argc, argv, "m:t:p:", long_options,
			&long_index)) != -1) {
		switch (opt) {
			case 'm':
				parse_comm_mode(optarg);
				break;
			case 't':
				parse_thread_mode(optarg);
				break;
			case 'p':
				parse_sched(optarg);
				break;
			default:
				usage();
				exit(-1);
				break;
		}
	}


	if (optind == argc - 2) { /* should contain the cpu #s */
		config.cpu1 = strtol(argv[optind++], NULL, 10);
		config.cpu2 = strtol(argv[optind++], NULL, 10);
		printf("Setting affinity to cpus %d and %d\n", config.cpu1, config.cpu2);
	}
	return 0;
}

inline void show_stats(int z) {
	static unsigned long long last_count = 0;
	unsigned long long current_count = stats->ping_count;
	unsigned long long interval_count = current_count - last_count;

	static long double last_stats_time = 0;
	long double current_stats_time = get_time();
	long double interval_time;

	if (last_stats_time == 0)
		last_stats_time = stats->start_time;

	interval_time = current_stats_time - last_stats_time;
	printf ("%llu iterations -> %s\n", interval_count, subsec_string(NULL, interval_time / interval_count, 3));
	if (current_stats_time >= stats->timeout_time)
		stats->stop = true;

	last_count = current_count;
	last_stats_time = current_stats_time;
}


inline int do_send(int fd) {
	return write(fd, "", 1);
}

inline int do_recv(int fd) {
	char dummy;

	return read(fd, &dummy, 1);
}

void stop_handler(int signum) {
	struct itimerval ntimeout;

//	printf("Got the stop signal\n");

	ntimeout.it_interval.tv_sec = ntimeout.it_interval.tv_usec = 0;
	ntimeout.it_value.tv_sec  = ntimeout.it_value.tv_usec = 0;
	signal(SIGALRM, SIG_IGN);
	setitimer(ITIMER_REAL,&ntimeout,NULL);	/* now stop timer */

	stats->stop = true;
}

int setup_timer() {
	struct sigaction sa;
	struct itimerval timer;

	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &show_stats;

	sigaction(SIGALRM, &sa, NULL);
	setitimer(ITIMER_REAL, &timer, 0);

	return 0;
}
int setup_stop_signal() {
	struct sigaction sa;

	stats->stop = false;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &stop_handler;
	sigaction(SIGINT, &sa, NULL);

	return 0;
}
void make_pairs() {
	config.make_pair(config.pair1);
	config.make_pair(config.pair2);

	config.parent_mouth = config.pair1[1];
	config.parent_ear = config.pair2[0];
	config.child_mouth = config.pair2[1];
	config.child_ear = config.pair1[0];
}

void set_affinity(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

int rename_thread(char *thread_name) {
	char name[17];
	int ret;

	strncpy(name, thread_name, 16);
	name[16] = 0;
	if ((ret = prctl(PR_SET_NAME, (unsigned long)&name)) == -1) {
		printf("Failed to set thread name to '%s': %m\n", name);
		return -1;
	}
	return 0;
}

void do_parent_work() {
	if (config.cpu1 != -1) {
		set_affinity(config.cpu1);
	} else { /* affinity not set */
		printf ("Affinity not set, however parent *currently* running on cpu %d\n", sched_getcpu());
	}

	setup_timer();
	setup_stop_signal();
	stats->start_time = get_time();
	stats->timeout_time = stats->start_time + config.max_execution_time * 1.0;

	while (stats->stop != true) {
		stats->ping_count ++;

		while ((config.do_send(config.parent_mouth) != 1) && (stats->stop != true));
		while ((config.do_recv(config.parent_ear) != 1) && (stats->stop != true));
	}

	/* cleanup */
	config.cleanup();
}

void do_child_work() {
	rename_thread("pong-thread");

	if (config.cpu2 != -1) {
		set_affinity(config.cpu2);
//		printf("Affinity set for child to cpu %d\n", config.cpu2);
	} else {
		printf("Affinity not set, however child *currently* running on cpu %d\n", sched_getcpu());
	}

	while (stats->stop != true) {
		while ((config.do_recv(config.child_ear) != 1) && (stats->stop != true));
		while ((config.do_send(config.child_mouth) != 1) && (stats->stop != true));
	}
}
int child_thread_function(void *argument) {
	do_child_work();
	return 0;
}
void *child_pthread_function(void *argument) {
	do_child_work();
	return NULL;
}

int do_fork() {

	stats->pong_pid = fork();
	if (stats->pong_pid > 0) {

		do_parent_work();
		kill(stats->pong_pid, SIGINT);
	} else if (stats->pong_pid == 0) { /* child process */
		do_child_work();
	} else {
		printf("Error while trying to fork: %d: %m\n", stats->pong_pid);
	}

	return 0;
}
int do_clone() {
	pid_t child;
	void *stack;

	stack = malloc(STACK_SIZE);
	if (stack == 0) {
		perror("malloc: could not allocate stack");
		exit(1);
	}

	child = clone(&child_thread_function,
		(char *) stack + STACK_SIZE,
		SIGCHLD | CLONE_FS | CLONE_FILES | \
		CLONE_SIGHAND | CLONE_VM,
		NULL);
	if (child == -1) {
		perror("clone");
		exit(2);
	}
	do_parent_work();
	free(stack);

	return 0;
}

int do_pthread() {
	pthread_t child_thread;

	pthread_create(&child_thread, NULL, &child_pthread_function, NULL);
	do_parent_work();

	return 0;
}

int start_threads() {
	if (config.thread_mode == thread_mode_fork) {
		do_fork();
	} else if (config.thread_mode == thread_mode_thread) {
		do_clone();
	} else if (config.thread_mode == thread_mode_pthread) {
		do_pthread();
	} else {
		printf("No thread mode specified?\n");
		exit(-1);
	}

	return 0;
}

int do_setup() {

	stats = mmap(NULL, sizeof(struct stats_struct),
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	make_pairs();

	printf("Will run for %lu seconds\n", config.max_execution_time);

#ifdef SET_PRIORITIES
	set_priorities();
#endif

	return 0;
}

int main(int argc, char *argv[]) {

	config.argv0 = argv[0];


	parse_opts(argc, argv);

	do_setup();

	start_threads();

	return 0;
}
