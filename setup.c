#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"

#include "comms.h"


#include "setup.h"
#include "sched.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/mman.h>

struct config_struct config;
struct run_data_struct *run_data;
struct control_struct *control_data;


/*
typedef enum { thread_mode_fork, thread_mode_thread, thread_mode_pthread, thread_mode_context } thread_modes;
*/
const char * thread_mode_strings[] = {
	[ thread_mode_fork ] = "fork"
	, [ thread_mode_thread ] = "threads"
	, [ thread_mode_pthread ] = "pthread"
};

static int usage() {
	int i;

	printf("Usage: %s [options] [ <cpu #> <cpu #> ]\n", config.argv0);
	printf("\n");
	printf("Options:\n");
	printf("\t-m, --mode=MODE\n");
//	printf("\t\tcommunication modes: tcp, udp, pipe, sockpair"
	printf("\t\tcommunication modes:\n");

/*	printf("\t\tcommunication modes: tcp, udp, pipe, sockpair"
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
*/

	for (i = 0 ; i < comm_mode_count ; i ++) {
		printf("\t\t\t%s\t%s\n", comm_mode_info[i].name, comm_mode_info[i].help_text ? : "");
	}

	printf("\n");



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



int setup_defaults(char *argv0) {
/* default settings */
	config.argv0 = argv0;

	config.monitor_check_frequency	= DEFAULT_MONITOR_CHECK_FREQ;
	config.max_execution_time	= DEFAULT_EXECUTION_TIME;
	config.stats_interval	= DEFAULT_STATS_INTERVAL;
	config.thread_mode	= DEFAULT_THREAD_MODE;

	config.comm_mode_index	= 0; /* default to whatever is first */
	config.comm_do_send	= comm_do_send;
	config.comm_do_recv	= comm_do_recv;
	config.comm_cleanup	= comm_no_cleanup;

	config.sched_policy	= DEFAULT_SCHED;
	config.sched_prio	= DEFAULT_SCHED_PRIO;

	config.uid = getuid();
	config.gid = getgid();
	config.euid = geteuid();
	config.egid = getegid();
	config.num_cpus = (short)num_cpus();
	config.num_online_cpus = (short)num_online_cpus();

	config.min_stack = get_min_stack_size();


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

static void make_pairs() {
	config.comm_make_pair(config.pair1);
	config.comm_make_pair(config.pair2);

	config.mouth[0] = config.pairs[0][1];
	config.ear[0] = config.pairs[1][0];
	config.mouth[1] = config.pairs[1][1];
	config.ear[1] = config.pairs[0][0];
}

int do_comm_setup() {

	printf("Setting up '%s'\n", get_comm_mode_name(config.comm_mode_index));

	config.comm_init = comm_mode_info[config.comm_mode_index].comm_init;
	config.comm_make_pair = comm_mode_info[config.comm_mode_index].comm_make_pair;
	config.comm_pre = comm_mode_info[config.comm_mode_index].comm_pre;
	config.comm_do_send = comm_mode_info[config.comm_mode_index].comm_do_send;
	config.comm_do_recv = comm_mode_info[config.comm_mode_index].comm_do_recv;
	config.comm_interrupt = comm_mode_info[config.comm_mode_index].comm_interrupt;
	config.comm_cleanup = comm_mode_info[config.comm_mode_index].comm_cleanup;

	run_data = mmap(NULL, sizeof(struct run_data_struct),
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	make_pairs();

	if (config.verbosity >= 0)
		printf("Will run for %lu seconds\n", config.max_execution_time);

	set_priorities();

	return 0;
}

