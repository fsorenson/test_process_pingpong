#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"

#include "setup.h"
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct config_struct config;
volatile struct run_data_struct *run_data;

struct comm_mode_info_struct {
	comm_modes comm_mode;
	char *name;

	int (*setup_comm)();
	int (*make_pair)(int fd[2]);
	int (*do_send)(int s);
	int (*do_recv)(int s);
	int (*cleanup)();
};

int no_setup() {
	return 0;
}
inline int do_send(int fd) {
	return write(fd, "X", 1);
}
//extern int do_send(int fd);
int do_recv(int fd) {
	char dummy;

	return read(fd, &dummy, 1);
}

int no_cleanup() {
	return 0;
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
#ifdef HAVE_EVENTFD
	, { comm_mode_eventfd, "eventfd", NULL, &make_eventfd_pair, &do_send_eventfd, &do_recv_eventfd, NULL }
	, { comm_mode_eventfd, "e", NULL, &make_eventfd_pair, &do_send_eventfd, &do_recv_eventfd, NULL }
#endif
	, { comm_mode_sem, "sem", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_sem, "sema", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_sem, "semaphore", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_busy_sem, "busy", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_busy_sem, "busysem", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_busy_sem, "busy_sem", NULL, &make_sem_pair, &do_send_sem, &do_recv_sem, &cleanup_sem }
	, { comm_mode_futex, "futex", NULL, &make_futex_pair, do_send_futex, do_recv_futex, NULL }
	, { comm_mode_futex, "f", NULL, &make_futex_pair, do_send_futex, do_recv_futex, NULL }
	, { comm_mode_spin, "spin", NULL, &make_spin_pair, do_send_spin, do_recv_spin, NULL }
	, { comm_mode_nop, "nop", NULL, &make_nop_pair, &do_send_nop, &do_recv_nop, NULL }
	, { comm_mode_mq, "mq", NULL, &make_mq_pair, &do_send_mq, &do_recv_mq, &cleanup_mq }
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
	unsigned int i;
	int found = -1;

	for (i = 0 ; i < sizeof(comm_mode_info) / sizeof(struct comm_mode_info_struct) ; i ++) {
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
	int minstack;

	if ((minstack = sysconf(_SC_THREAD_STACK_MIN)) == -1) {
		perror("sysconf(_SC_THREAD_STACK_MIN)");
		exit(1);
	}
	return minstack;
}

int setup_defaults(char *argv0) {
/* default settings */
	config.argv0 = argv0;

	config.max_execution_time = DEFAULT_EXECUTION_TIME;
	config.stats_interval = DEFAULT_STATS_INTERVAL;
	config.thread_mode = DEFAULT_THREAD_MODE;

	config.comm_mode = DEFAULT_COMM_MODE;
	config.do_send = &do_send;
	config.do_recv = &do_recv;
	config.cleanup = &no_cleanup;

	config.sched_policy = DEFAULT_SCHED;
	config.sched_prio = DEFAULT_SCHED_PRIO;

	config.uid = getuid();
	config.gid = getgid();
	config.euid = geteuid();
	config.egid = getegid();
	config.num_cpus = num_cpus();
	config.num_online_cpus = num_online_cpus();

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
		{	"tcp",		no_argument,		&config.comm_mode_i, comm_mode_tcp },
		{	"udp",		no_argument,		&config.comm_mode_i, comm_mode_udp },
		{	"pipe",		no_argument,		&config.comm_mode_i, comm_mode_pipe },
		{	"sockpair",	no_argument,		&config.comm_mode_i, comm_mode_sockpair },
#ifdef HAVE_EVENTFD
		{	"eventfd",	no_argument,		&config.comm_mode_i, comm_mode_eventfd },
#endif
		{	"sem",		no_argument,		&config.comm_mode_i, comm_mode_sem },
		{	"sema",		no_argument,		&config.comm_mode_i, comm_mode_sem },
		{	"semaphore",	no_argument,		&config.comm_mode_i, comm_mode_sem },
		{	"busy_sem",	no_argument,		&config.comm_mode_i, comm_mode_busy_sem },
		{	"busy",		no_argument,		&config.comm_mode_i, comm_mode_busy_sem },

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
		config.cpu[0] = strtol(argv[optind++], NULL, 10);
		config.cpu[1] = strtol(argv[optind++], NULL, 10);
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

void set_affinity(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

//static int run_data_shm_id;

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

int do_setup() {
	unsigned int i;
	int found = -1;

	for (i = 0 ; i < sizeof(comm_mode_info) / sizeof(struct comm_mode_info_struct) ; i ++) {
		if (config.comm_mode == comm_mode_info[i].comm_mode) {
			found = i;
			break;
		}
	}
	if (found < 0) {
		printf("Unable to configure the communications correctly\n");
		exit(-1);
	}

	config.setup_comm = &no_setup;
	config.make_pair = comm_mode_info[found].make_pair;
	config.do_send = comm_mode_info[found].do_send != NULL ? comm_mode_info[found].do_send : &do_send;
	config.do_recv = comm_mode_info[found].do_recv != NULL ? comm_mode_info[found].do_recv : &do_recv;
	config.cleanup = comm_mode_info[found].cleanup != NULL ? comm_mode_info[found].cleanup : &no_cleanup;


// sharing the 'run_data' struct...
// if we use clone + CLONE_VM, we share an address space, so everything's shared
// if we don't use CLONE_VM, we could use shared memory or a shared mmap...
// don't know which is better
//
//	run_data_shm_id = shmget(IPC_PRIVATE, sizeof(struct run_data_struct), IPC_CREAT | 0600);
//	run_data = shmat(run_data_shm_id, NULL, 0);

//	if (config.thread_mode == thread_mode_thread) {
//		run_data = calloc(1, sizeof(struct run_data_struct));
//	} else {
		run_data = mmap(NULL, sizeof(struct run_data_struct),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//	}


	make_pairs();

	if (config.verbosity >= 0)
		printf("Will run for %lu seconds\n", config.max_execution_time);

	set_priorities();

	return 0;
}

