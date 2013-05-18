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

static int usage(void) {
	int i;

	printf(
"Usage: %s [options] [ <cpu #> <cpu #> ]\n", config.argv0);
printf(
"\n"
"Options:\n"

"    -u, --update=#      seconds between statistics display (0 for no intermediate stats--implies '-s'); default=" DEFAULT_STATS_INTERVAL_STRING "\n"
"    -r, --runtime=#     number of seconds to run the test (0 to run continuously); default=" xstr(DEFAULT_EXECUTION_TIME) "\n"
"    -s, --stats         output overall statistics at the end of the run (default)\n"
"        --nostats       do not output statistics at the end of the run\n"
"    -m, --mode=MODE\n"
"        communication modes:\n"
);

	for (i = 0 ; i < comm_mode_count ; i ++) {
		printf(
"            %15s  -  %s\n", comm_mode_info[i].name,
		comm_mode_info[i].help_text ? comm_mode_info[i].help_text : "");
	}

	printf(
"\n"
"\n"
"    -t, --thread=MODE, --thread_mode=MODE\n"
"        thread modes: fork, clone, pthread\n"
"\n"
"    -p, --priority=POLICY:PRIORITY\n"
"        policies: fifo, rr\n"
"        priorities: 1-99 (careful with high priorities)\n"
"\n"
"    <cpu #> - pin the threads to the designated CPUs\n"
);

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
	config.runtime		= DEFAULT_EXECUTION_TIME;
	config.stats_interval	= DEFAULT_STATS_INTERVAL;
	config.stats_headers_frequency	= DEFAULT_STATS_HEADERS_FREQ;
	config.thread_mode	= DEFAULT_THREAD_MODE;

	config.comm_mode_index	= 0; /* default to whatever is first */
	config.comm_do_ping	= comm_do_ping_generic;
	config.comm_do_pong	= comm_do_pong_generic;
	config.comm_do_send	= comm_do_send_generic;
	config.comm_do_recv	= comm_do_recv_generic;
	config.comm_cleanup	= comm_no_cleanup;

	config.cpu_dma_latency	= DEFAULT_CPU_LATENCY_VALUE;
	config.sched_policy	= DEFAULT_SCHED;
	config.sched_prio	= DEFAULT_SCHED_PRIO;

//	config.size_align_flag	= SIZE_ALIGN_NONE;
//	config.size_align_flag	= SIZE_ALIGN_NORMAL;
	config.size_align_flag	= SIZE_ALIGN_HUGE;

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
	long double arg_ld;
	long long arg_ll;

	static struct option long_options[] = {
		{	"runtime",	required_argument,	0,	'r'	}, /* seconds to run the entire test */
		{	"update",	required_argument,	0,	'u'	}, /* seconds between stats output */
		{	"mode",		required_argument,	0,	'm'	}, /* communication mode */

		{	"thread",	required_argument,	0,	't'	},
		{	"thread_mode",	required_argument,	0,	't'	},
		{	"sched",	required_argument,	0,	'p'	},
		{	0,		0,			0,	0	}
	};

	opterr = 0;
	while ((opt = getopt_long(argc, argv, "m:t:p:r:u:", long_options,
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
			case 'r':
				config.runtime = strtoul(optarg, NULL, 10);
				break;
			case 'u':
				arg_ld = strtod(optarg, NULL);
				if (arg_ld < 0.0L) {
					config.stats_interval.tv_sec = DEFAULT_STATS_INTERVAL.tv_sec;
					config.stats_interval.tv_usec = DEFAULT_STATS_INTERVAL.tv_usec;
				} else if (fpclassify(arg_ld) == FP_ZERO) {
					config.stats_summary = true;
				} else {
					arg_ll = llrintl(arg_ld * 1.0e6L);
					config.stats_interval.tv_sec = arg_ll / 1000000;
					config.stats_interval.tv_usec = arg_ll % 1000000;
				}
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

static void make_pairs(void) {
	config.comm_make_pair(config.pair1);
	config.comm_make_pair(config.pair2);

	config.mouth[0] = config.pairs[0][1];
	config.ear[0] = config.pairs[1][0];
	config.mouth[1] = config.pairs[1][1];
	config.ear[1] = config.pairs[0][0];
}

int do_comm_setup(void) {

	printf("Setting up '%s'\n", get_comm_mode_name(config.comm_mode_index));

	config.comm_init = comm_mode_info[config.comm_mode_index].comm_init;
	config.comm_make_pair = comm_mode_info[config.comm_mode_index].comm_make_pair;
	config.comm_pre = comm_mode_info[config.comm_mode_index].comm_pre;
	config.comm_begin = comm_mode_info[config.comm_mode_index].comm_begin;
	config.comm_do_ping = comm_mode_info[config.comm_mode_index].comm_do_ping;
	config.comm_do_pong = comm_mode_info[config.comm_mode_index].comm_do_pong;
	config.comm_do_send = comm_mode_info[config.comm_mode_index].comm_do_send;
	config.comm_do_recv = comm_mode_info[config.comm_mode_index].comm_do_recv;
	config.comm_cleanup = comm_mode_info[config.comm_mode_index].comm_cleanup;

	run_data = map_shared_area(sizeof(struct run_data_struct), config.size_align_flag);

	make_pairs();

	if (config.verbosity >= 0)
		printf("Will run for %lu seconds\n", config.runtime);

	set_priorities();

	return 0;
}

