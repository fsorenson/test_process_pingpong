

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <getopt.h>
#include <pthread.h>



// 64 KB stack
#define STACK_SIZE 1024*64


typedef enum { comm_mode_tcp, comm_mode_udp, comm_mode_pipe, comm_mode_sockpair, comm_mode_eventfd } comm_modes;
const char * comm_mode_strings[] = { "tcp", "udp", "pipe", "sockpair", "eventfd" };
typedef enum { thread_mode_fork, thread_mode_thread, thread_mode_pthread, thread_mode_context } thread_modes;
const char * thread_mode_strings[] = { "fork", "threads", "pthread" };

/*
  thread_mode_context - makecontext, ucontext.h, etc.
  also, look into setjmp - see http://www.evanjones.ca/software/threading.html
  also, see http://syprog.blogspot.com/2012/03/linux-threads-through-magnifier-local.html
*/

unsigned int volatile running_count = 0;

struct config_struct {
	char *argv0;
	comm_modes comm_mode;
	thread_modes thread_mode;

	int cpu1;
	int cpu2;

	int pairs[2][2];
	int *pair1;
	int *pair2;
//	int pair1[2];
//	int pair2[2];
	int parent_mouth;
	int parent_ear;
	int child_mouth;
	int child_ear;

	int (*make_pair)(int fd[2]);
	int (*do_send)(int s);
	int (*do_recv)(int s);
} config;

int make_tcp_pair(int fd[2]);
int make_udp_pair(int fd[2]);
int make_socket_pair(int fd[2]);
int make_eventfd_pair(int fd[2]);
int do_send(int fd);
int do_recv(int fd);
int do_send_eventfd(int fd);
int do_recv_eventfd(int fd);

int usage() {
	printf("Usage: %s [options] [ <cpu #> <cpu #> ]\n", config.argv0);
	printf("\n");
	printf("Options:\n");
	printf("\t-m, --mode=MODE\n");
	printf("\t\tcommunication modes: tcp, udp, pipe, sockpair, eventfd\n");
	printf("\t-t, --thread=MODE, --thread_mode=MODE\n");
	printf("\t\tthread modes: fork, clone, pthread\n");

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
	printf("Using thread mode: %s\n", thread_mode_strings[config.thread_mode]);

	return 0;
}

int parse_comm_mode(char *arg) {
	if (! strcmp(arg, "tcp") || ! strcmp(arg, "t") )
		config.comm_mode = comm_mode_tcp;
	else if (! strcmp(arg, "udp") || ! strcmp(arg, "u") )
		config.comm_mode = comm_mode_udp;
	else if (! strcmp(arg, "pipe") || ! strcmp(arg, "p") )
		config.comm_mode = comm_mode_pipe;
	else if (! strcmp(arg, "sockpair") || ! strcmp(arg, "s") )
		config.comm_mode = comm_mode_sockpair;
	else if (! strcmp(arg, "eventfd") || ! strcmp(arg, "e") )
		config.comm_mode = comm_mode_eventfd;
	else {
		printf("Unknown communication mode '%s'\n", arg);
	}
	printf("Using communication_mode: %s\n", comm_mode_strings[config.comm_mode]);

	return 0;
}

int setup_comm_mode() {
	printf("Communicating via '%s'\n", comm_mode_strings[config.comm_mode]);
	switch (config.comm_mode) {
		case comm_mode_tcp: config.make_pair = &make_tcp_pair; break;
		case comm_mode_udp: config.make_pair = &make_udp_pair; break;
		case comm_mode_pipe: config.make_pair = &pipe; break;
		case comm_mode_sockpair: config.make_pair = &make_socket_pair; break;
		case comm_mode_eventfd:
			config.make_pair = &make_eventfd_pair;
			config.do_send = &do_send_eventfd;
			config.do_recv = &do_recv_eventfd;
			break;
	}

	return 0;
}


int parse_opts(int argc, char *argv[]) {
	int opt = 0, long_index = 0;

	/* default settings */
	config.comm_mode = comm_mode_tcp;
	config.do_send = &do_send;
	config.do_recv = &do_recv;

	config.thread_mode = thread_mode_fork;

	static struct option long_options[] = {
		{	"mode",		required_argument,	0,	'm'	}, /* communication mode */
		{	"thread",	required_argument,	0,	't'	},
		{	"thread_mode",	required_argument,	0,	't'	},
		{	0,		0,			0,	0	}
	};

	config.cpu1 = -1;
	config.cpu2 = -1;
	config.pair1 = config.pairs[0];
	config.pair2 = config.pairs[1];

	opterr = 0;
	while ((opt = getopt_long(argc, argv, "m:t:", long_options,
			&long_index)) != -1) {
		switch (opt) {
			case 'm':
				parse_comm_mode(optarg);
				break;
			case 't':
				parse_thread_mode(optarg);
				break;
			default:
				usage();
				exit(-1);
				break;
		}
	}
	setup_comm_mode();

	if (optind == argc - 2) { /* should contain the cpu #s */
		config.cpu1 = strtol(argv[optind++], NULL, 10);
		config.cpu2 = strtol(argv[optind++], NULL, 10);
		printf("Setting affinity to cpus %d and %d\n", config.cpu1, config.cpu2);
	}

	return 0;
}

void show_stats(int) {
	static unsigned long last_count = 0;
	unsigned long current_count = running_count;
	printf ("%ld iterations -> %f us\n", current_count - last_count, (1000000. / (current_count - last_count)));

	last_count = current_count;
}

int make_tcp_pair(int fd[2]) {
	int fds, fdc;
	socklen_t addr_len;

	union {
		struct sockaddr_in saddr_in;
		struct sockaddr saddr;
	};

	fds = socket(PF_INET, SOCK_STREAM, 0);

	memset(&saddr, 0, sizeof(saddr_in));
	saddr_in.sin_family = AF_INET;
	saddr_in.sin_port = 0;
//	saddr_in.sin_addr.s_addr = inet_addr("1.2.3.4");
	saddr_in.sin_addr.s_addr = INADDR_ANY;
//	saddr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
	bind(fds, &saddr, sizeof(saddr_in));
	listen(fds, 1);


	fdc = socket(PF_INET, SOCK_STREAM, 0);

	addr_len = sizeof(saddr);
	getsockname(fds, &saddr, &addr_len);
	connect(fdc, &saddr, addr_len);


	fd[0] = accept(fds, 0, 0);
	fd[1] = fdc;
	close(fds);
	return 0;
}

int make_udp_pair(int fd[2]) {
	int fds, fdc;
	socklen_t addr_len;

	union {
		struct sockaddr_in saddr_in;
		struct sockaddr saddr;
	};

	fds = socket(PF_INET, SOCK_DGRAM, 0);

	memset(&saddr, 0, sizeof(saddr_in));
	saddr_in.sin_family = AF_INET;
	saddr_in.sin_port = 0;
	saddr_in.sin_addr.s_addr = INADDR_ANY;
	bind(fds, &saddr, sizeof(saddr_in));

	fdc = socket(PF_INET, SOCK_DGRAM, 0);

	memset(&saddr, 0, sizeof(saddr_in));
	saddr_in.sin_family = AF_INET;
	saddr_in.sin_port = 0;
	saddr_in.sin_addr.s_addr = INADDR_ANY;
	bind(fds, &saddr, sizeof(saddr_in));


	addr_len = sizeof(saddr);
	getsockname(fds, &saddr, &addr_len);
	connect(fdc, &saddr, addr_len);

	addr_len = sizeof(saddr);
	getsockname(fdc, &saddr, &addr_len);
	connect(fds, &saddr, addr_len);

	fd[0] = fds;
	fd[1] = fdc;
	return 0;
}

int make_socket_pair(int fd[2]) {
	socketpair(AF_UNIX, SOCK_STREAM, AF_LOCAL, fd);
	return 0;
}

int make_eventfd_pair(int fd[2]) {
	fd[0] = eventfd(0, 0);
	fd[1] = dup(fd[0]);
	return 0;
}

int do_send(int fd) {
	return write(fd, "", 1);
}

int do_recv(int fd) {
	char dummy;

	return read(fd, &dummy, 1);
}

int do_send_eventfd(int fd) {
	return ! eventfd_write(fd, 1);
}
int do_recv_eventfd(int fd) {
	eventfd_t dummy;

	return ! eventfd_read(fd, &dummy);
}

void setup_timer() {
	sigset(SIGALRM, show_stats);
	struct itimerval timer = { { 1, 0 }, { 1, 0 }};
	setitimer(ITIMER_REAL, &timer, 0);
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

void do_parent_work() {
//	close(config.pair1[0]);
//	close(config.pair2[1]);

	if (config.cpu1 != -1) {
		set_affinity(config.cpu1);
	} else { /* affinity not set */
		printf ("Affinity not set, however parent *currently* running on cpu %d\n", sched_getcpu());
	}

	setup_timer();

	for (;;) {
		running_count ++;

		while (config.do_send(config.parent_mouth) != 1);
		while (config.do_recv(config.parent_ear) != 1);
	}
}

void do_child_work() {
//	close(config.pair1[1]);
//	close(config.pair2[0]);

	if (config.cpu2 != -1) {
		set_affinity(config.cpu2);
	} else {
		printf("Affinity not set, however child *currently* running on cpu %d\n", sched_getcpu());
	}

	for (;;) {
		while (config.do_recv(config.child_ear) != 1);
		while (config.do_send(config.child_mouth) != 1);
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
	pid_t child;

	child = fork();
	if (child) { /* parent process */
		do_parent_work();
	} else { /* child process */
		do_child_work();
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
	printf("Creating child thread\n");

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
	if (config.thread_mode == thread_mode_fork)
		do_fork();
	else if (config.thread_mode == thread_mode_thread)
		do_clone();
	else if (config.thread_mode == thread_mode_pthread)
		do_pthread();

	return 0;
}

int main(int argc, char *argv[]) {

	config.argv0 = argv[0];

	parse_opts(argc, argv);
	make_pairs();

	start_threads();


	return 0;
}




