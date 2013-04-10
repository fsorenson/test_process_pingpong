#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"

#include "setup.h"
#include "sched.h"

//#include "tcp.h"
#include "udp.h"
#include "pipe.h"
#include "socket_pair.h"
#include "sem.h"
#include "futex.h"
#include "spin.h"
#include "nop.h"
#include "mq.h"

#ifdef HAVE_EVENTFD
#include "eventfd.h"
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/mman.h>

struct config_struct config;
volatile struct run_data_struct *run_data;
volatile struct control_struct *control_data;

struct comm_mode_info_struct {
	comm_modes comm_mode;
	char *name;

	int (*comm_init)();
	int (*make_pair)(int fd[2]);
	int (*pre_comm)();		/* after fork/clone, but before communication begins */
	int (*do_send)(int s);
	int (*do_recv)(int s);
	int (*comm_interrupt)(int s);
	int (*comm_cleanup)();
};



struct comm_mode_info_struct comm_mode_info[] = {
	{
		.comm_mode	= comm_mode_tcp,
		.name		= "tcp",
		.make_pair	= make_tcp_pair,
	}
	, {
		.comm_mode	= comm_mode_udp,
		.name		= "udp",
		.make_pair	= make_udp_pair
	}
	, {
		.comm_mode	= comm_mode_pipe,
		.name		= "pipe",
		.pre_comm	= pre_comm_pipe,
		.make_pair	= pipe,
		.comm_cleanup	= comm_cleanup_pipe
	}
	, {
		.comm_mode	= comm_mode_sockpair,
		.name		= "sockpair",
		.make_pair	= make_socket_pair
	}
#ifdef HAVE_EVENTFD
	, {
		.comm_mode	= comm_mode_eventfd,
		.name		= "eventfd",
		.make_pair	= make_eventfd_pair,
		.do_send	= do_send_eventfd,
		.do_recv	= do_recv_eventfd
	}
#endif
	, {
		.comm_mode	= comm_mode_sem,
		.name		= "sem",
		.make_pair	= make_sem_pair,
		.do_send	= do_send_sem,
		.do_recv	= do_recv_sem,
		.comm_cleanup	= cleanup_sem
	}
	, {
		.comm_mode	= comm_mode_busy_sem,
		.name		= "busysem",
		.make_pair	= make_sem_pair,
		.do_send	= do_send_sem,
		.do_recv	= do_recv_sem,
		.comm_cleanup	= cleanup_sem
	}
	, {
		.comm_mode	= comm_mode_futex,
		.name		= "futex",
		.make_pair	= make_futex_pair,
		.do_send	= do_send_futex,
		.do_recv	= do_recv_futex
	}
	, {
		.comm_mode	= comm_mode_spin,
		.name		= "spin",
		.make_pair	= make_spin_pair,
		.do_send	= do_send_spin,
		.do_recv	= do_recv_spin
	}
	, {
		.comm_mode	= comm_mode_nop,
		.name		= "nop",
		.make_pair	= make_nop_pair,
		.do_send	= do_send_nop,
		.do_recv	= do_recv_nop
	}
	, {
		.comm_mode	= comm_mode_mq,
		.name		= "mq",
		.make_pair	= make_mq_pair,
		.do_send	= do_send_mq,
		.do_recv	= do_recv_mq,
		.comm_cleanup	= cleanup_mq
	}
};

/*
typedef enum { thread_mode_fork, thread_mode_thread, thread_mode_pthread, thread_mode_context } thread_modes;
*/
const char * thread_mode_strings[] = {
	[ thread_mode_fork ] = "fork"
	, [ thread_mode_thread ] = "threads"
	, [ thread_mode_pthread ] = "pthread"
};

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
	if (config.verbosity >= 1) {
		printf("thread mode requested was '%s'\n", arg);
		printf("Using thread mode: %s\n", thread_mode_strings[config.thread_mode]);
	}

	return 0;
}

char *get_comm_mode_name(unsigned int mode) {
	unsigned int i;

	for (i = 0 ; i < sizeof(comm_mode_info) / sizeof(struct comm_mode_info_struct) ; i ++) {
		if (mode == comm_mode_info[i].comm_mode)
			return comm_mode_info[i].name;
	}
	return "UNKNOWN";
}

int parse_comm_mode(char *arg) {
	int i;
	int found = -1;

	for (i = 0 ; i < (int)(sizeof(comm_mode_info) / sizeof(struct comm_mode_info_struct)) ; i ++) {
		if (! strcmp(arg, comm_mode_info[i].name)) {
			found = i;
			break;
		}
	}
	if (found == -1) {
		printf("Unknown communication mode '%s'.  Falling back to default\n", arg);
//		return -1;
	} else {
		config.comm_mode = comm_mode_info[found].comm_mode;
	}

	return 0;
}

int min_stack_size() {
	long int minstack;

	if ((minstack = sysconf(_SC_THREAD_STACK_MIN)) == -1) {
		perror("sysconf(_SC_THREAD_STACK_MIN)");
		exit(1);
	}
	return (int)minstack;
}

int setup_defaults(char *argv0) {
/* default settings */
	config.argv0 = argv0;

	config.max_execution_time	= DEFAULT_EXECUTION_TIME;
	config.stats_interval	= DEFAULT_STATS_INTERVAL;
	config.thread_mode	= DEFAULT_THREAD_MODE;

	config.comm_mode	= DEFAULT_COMM_MODE;
	config.do_send		= do_send;
	config.do_recv		= do_recv;
	config.comm_cleanup	= no_comm_cleanup;

	config.sched_policy	= DEFAULT_SCHED;
	config.sched_prio	= DEFAULT_SCHED_PRIO;

	config.uid = getuid();
	config.gid = getgid();
	config.euid = geteuid();
	config.egid = getegid();
	config.num_cpus = (short)num_cpus();
	config.num_online_cpus = (short)num_online_cpus();

	config.min_stack = min_stack_size();


	config.cpu[0] = -1;
	config.cpu[1] = -1;
	config.pair1 = config.pairs[0];
	config.pair2 = config.pairs[1];

	return 0;
}

int parse_opts(int argc, char *argv[]) {
	int opt = 0, long_index = 0;

	static struct option long_options[] = {
		{	"mode",		required_argument,	0,	'm'	}, /* communication mode */
		{	"tcp",		no_argument,	(int *)	&config.comm_mode, comm_mode_tcp },
		{	"udp",		no_argument,	(int *)	&config.comm_mode, comm_mode_udp },
		{	"pipe",		no_argument,	(int *)	&config.comm_mode, comm_mode_pipe },
		{	"sockpair",	no_argument,	(int *)	&config.comm_mode, comm_mode_sockpair },
#ifdef HAVE_EVENTFD
		{	"eventfd",	no_argument,	(int *)	&config.comm_mode, comm_mode_eventfd },
#endif
		{	"sem",		no_argument,	(int *)	&config.comm_mode, comm_mode_sem },
		{	"sema",		no_argument,	(int *)	&config.comm_mode, comm_mode_sem },
		{	"semaphore",	no_argument,	(int *)	&config.comm_mode, comm_mode_sem },
		{	"busy_sem",	no_argument,	(int *)	&config.comm_mode, comm_mode_busy_sem },
		{	"busy",		no_argument,	(int *)	&config.comm_mode, comm_mode_busy_sem },

		{	"thread",	required_argument,	0,	't'	},
		{	"thread_mode",	required_argument,	0,	't'	},
		{	"sched",	required_argument,	0,	'p'	},
		{	0,		0,			0,	0	}
	};


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
		config.set_affinity = true;
		config.cpu[0] = (short)strtol(argv[optind++], NULL, 10);
		config.cpu[1] = (short)strtol(argv[optind++], NULL, 10);
		printf("Setting affinity to cpus %d and %d\n", config.cpu[0], config.cpu[1]);
	}
	return 0;
}

void make_pairs() {
	config.make_pair(config.pair1);
	config.make_pair(config.pair2);

	config.mouth[0] = config.pairs[0][1];
	config.ear[0] = config.pairs[1][0];
	config.mouth[1] = config.pairs[1][1];
	config.ear[1] = config.pairs[0][0];
}

//static int run_data_shm_id;


int do_comm_setup() {
	int i;
	int found = -1;

	for (i = 0 ; i < (int) (sizeof(comm_mode_info) / sizeof(struct comm_mode_info_struct)) ; i ++) {
		if (config.comm_mode == comm_mode_info[i].comm_mode) {
			found = i;
			break;
		}
	}
	if (found < 0) {
		printf("Unable to configure the communications correctly\n");
		exit(-1);
	}

	config.comm_init = comm_mode_info[found].comm_init != NULL ?
		comm_mode_info[found].comm_init : no_comm_init;
	config.make_pair = comm_mode_info[found].make_pair;
	config.pre_comm = comm_mode_info[found].pre_comm != NULL ?
		comm_mode_info[found].pre_comm : no_pre_comm;
	config.do_send = comm_mode_info[found].do_send != NULL ?
		comm_mode_info[found].do_send : do_send;
	config.do_recv = comm_mode_info[found].do_recv != NULL ?
		comm_mode_info[found].do_recv : do_recv;
	config.comm_interrupt = comm_mode_info[found].comm_interrupt != NULL ?
		comm_mode_info[found].comm_interrupt : no_comm_interrupt;
	config.comm_cleanup = comm_mode_info[found].comm_cleanup != NULL ?
		comm_mode_info[found].comm_cleanup : no_comm_cleanup;


	run_data = mmap(NULL, sizeof(struct run_data_struct),
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	make_pairs();

	if (config.verbosity >= 0)
		printf("Will run for %lu seconds\n", config.max_execution_time);

	set_priorities();

	return 0;
}

