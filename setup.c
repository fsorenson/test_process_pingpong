/*
	This file is part of test_process_pingpong

	Copyright 2013 Frank Sorenson (frank@tuxrocks.com)


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "setup.h"
#include "comms.h"
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

static void usage(void) {
	int i;

	printf(
"Usage: %s [options] [ <cpu #> <cpu #> ]\n", config.argv0);
printf(
"\n"
"Options:\n"

"    -u, --update=#      seconds between statistics display (0 for no intermediate stats--implies '-s'); default=" DEFAULT_STATS_INTERVAL_STRING "\n"
"    -r, --runtime=#     number of seconds to run the test (0 to run continuously); default=" __XSTR(DEFAULT_EXECUTION_TIME) "\n"
"    -s, --stats         output overall statistics at the end of the run (default)\n"
"        --nostats       do not output statistics at the end of the run\n"
"    -l, --latency=#	 set the cpu_dma_latency value; default=none\n"
"        --secret_sauce  enable the advanced 'secret sauce' setting\n"
"    -o, --option=       set a communication-mode-specific option (for modes with options)\n"
"    -m, --mode=MODE\n"
"        communication modes:\n"
);

	for (i = 0 ; i < comm_mode_count ; i ++) {
		printf(
"            %15s  -  %s\n", comm_mode_info[i].name,
		comm_mode_info[i].help_text ? comm_mode_info[i].help_text : "");
		if (comm_mode_info[i].comm_show_options != NULL) {
			comm_mode_info[i].comm_show_options("\t\t\t\t");
		}
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
}

int parse_thread_mode(char *arg) {
	if (strcmp(arg, "fork") == 0 )
		config.thread_mode = thread_mode_fork;
	else if (strcmp(arg, "pthread") == 0)
		config.thread_mode = thread_mode_pthread;
	else if (strcmp(arg, "clone") == 0)
		config.thread_mode = thread_mode_thread;
	else if (strcmp(arg, "thread") == 0)
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

void setup_defaults(char *argv0) {
/* default settings */
	config.argv0 = argv0;

	config.output_fd	= DEFAULT_OUTPUT_FD;

	config.monitor_check_frequency	= DEFAULT_MONITOR_CHECK_FREQ;
	config.runtime		= DEFAULT_EXECUTION_TIME;
	config.stats_interval	= DEFAULT_STATS_INTERVAL;
	config.stats_headers_frequency	= DEFAULT_STATS_HEADERS_FREQ;
	config.thread_mode	= DEFAULT_THREAD_MODE;

	config.comm_mode_index	= 0; /* default to whatever is first */
	config.comm_ping	= comm_ping_generic;
	config.comm_pong	= comm_pong_generic;
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
}

void parse_opts(int argc, char *argv[]) {
	int opt = 0, long_index = 0;
	char *comm_option_string = 0;
	long double arg_ld;
	long long arg_ll;
	long arg_l;

	static struct option long_options[] = {
		{	"runtime",	required_argument,	0,	'r'	}, /* seconds to run the entire test */
		{	"update",	required_argument,	0,	'u'	}, /* seconds between stats output */
		{	"mode",		required_argument,	0,	'm'	}, /* communication mode */

		{	"thread",	required_argument,	0,	't'	},
		{	"thread_mode",	required_argument,	0,	't'	},
		{	"sched",	required_argument,	0,	'p'	},
		{	"latency",	required_argument,	0,	'l'	},
		{	"option",	required_argument,	0,	'o'	},
		{	"secret_sauce",	no_argument,		0,	'z'	},
		{	0,		0,			0,	0	}
	};

	opterr = 0;
	while ((opt = getopt_long(argc, argv, "m:t:p:r:u:l:zo:", long_options,
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
			case 'z':
				config.cpu_dma_latency = 0;
				break;
			case 'o':
				comm_option_string = optarg;
				break;
			case 'l':
				arg_l = strtol(optarg, NULL, 10);
				if (arg_l < -1)
					arg_l = -1;
				config.cpu_dma_latency = arg_l;
				break;
			default:
				usage();
				exit(EXIT_FAILURE);
				break;
		}
	}
	if (comm_option_string != NULL) {
		comm_mode_info[config.comm_mode_index].comm_parse_options(comm_option_string);
	}

	if (optind == argc - 2) { /* should contain the cpu #s */
		config.set_affinity = true;
		config.cpu[0] = (short)strtol(argv[optind++], NULL, 10);
		config.cpu[1] = (short)strtol(argv[optind++], NULL, 10);
	}
}

static void make_pairs(void) {
	config.comm_make_pair(config.pair1);
	config.comm_make_pair(config.pair2);

	config.mouth[0] = config.pairs[0][1];
	config.ear[0] = config.pairs[1][0];
	config.mouth[1] = config.pairs[1][1];
	config.ear[1] = config.pairs[0][0];
}

void do_comm_setup(void) {
	if (config.verbosity >= 0) {
		printf("Configuring to run ");
		if (config.runtime > 0)
			printf("for %lu seconds", config.runtime);
		else
			printf("indefinitely");

		printf(" in '%s' mode", get_comm_mode_name(config.comm_mode_index));

		if (config.set_affinity == true)
			printf(", with threads bound to cpus %d and %d", config.cpu[0], config.cpu[1]);
		else
			printf(", with no cpu affinity");
		printf("\n");
	}

	config.cpu_mhz = estimate_cpu_mhz();
	config.cpu_cycle_time = 1 / config.cpu_mhz / 1e6L;

	config.comm_init = comm_mode_info[config.comm_mode_index].comm_init;
	config.comm_make_pair = comm_mode_info[config.comm_mode_index].comm_make_pair;
	config.comm_pre = comm_mode_info[config.comm_mode_index].comm_pre;
	config.comm_begin = comm_mode_info[config.comm_mode_index].comm_begin;
	config.comm_ping = comm_mode_info[config.comm_mode_index].comm_ping;
	config.comm_pong = comm_mode_info[config.comm_mode_index].comm_pong;
	config.comm_do_send = comm_mode_info[config.comm_mode_index].comm_do_send;
	config.comm_do_recv = comm_mode_info[config.comm_mode_index].comm_do_recv;
	config.comm_cleanup = comm_mode_info[config.comm_mode_index].comm_cleanup;

	run_data = map_shared_area(sizeof(struct run_data_struct), config.size_align_flag);

	make_pairs();

	set_priorities();
}

