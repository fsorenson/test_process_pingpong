/*
        test_process_pingpong: program to test 'ping-pong' performance
		between two processes on a host

        by Frank Sorenson (frank@tuxrocks.com) 2013
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "setup.h"

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
#include <sys/wait.h>
#include <execinfo.h>

#include "units.h"
#include "sched.h"


long double get_time() {
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return (ts.tv_sec * 1.0) + (ts.tv_nsec / 1000000000.0);
}


void print_backtrace(int signum) {
  printf("print_backtrace: got signal %d\n", signum);

  void           *array[32];    /* Array to store backtrace symbols */
  size_t          size;     /* To store the exact no of values stored */
  char          **strings;    /* To store functions from the backtrace list in ARRAY */
  size_t          nCnt;

  size = backtrace(array, 32);

  strings = backtrace_symbols(array, size);

  /* prints each string of function names of trace*/
  for (nCnt = 0; nCnt < size; nCnt++)
    fprintf(stderr, "%s\n", strings[nCnt]);

  exit(-1);
}

void print_backtrace2(int signum) {
(void)signum;
        pid_t dying_pid = getpid();
        pid_t child_pid = fork();
        if (child_pid < 0) {
                perror("fork() while collecting backtrace:");
        } else if (child_pid == 0) {
                char buf[1024];
                sprintf(buf, "gdb -p %d -batch -ex thread apply all bt 2>/dev/null | "
                "sed '0,/<signal handler/d'", dying_pid);
                const char* argv[] = {"sh", "-c", buf, NULL};
                execve("/bin/sh", (char**)argv, NULL);
                _exit(1);
        } else {
                waitpid(child_pid, NULL, 0);
        }
        _exit(1);
}



void show_stats(int signum) {
	(void)signum;

	static char buffer[256];


	static unsigned long long last_count = 0;
	unsigned long long current_count = stats->ping_count;
	unsigned long long interval_count = current_count - last_count;

	static long double last_stats_time = 0;
	long double current_stats_time = get_time();
	long double interval_time;

	if (last_stats_time == 0)
		last_stats_time = stats->start_time;

	interval_time = current_stats_time - last_stats_time;
	snprintf(buffer, 255, "%llu iterations -> %s\n", interval_count, subsec_string(NULL, interval_time / interval_count, 3));
	buffer[255] = 0;
	write(1, buffer, strlen(buffer));
	if (current_stats_time >= stats->timeout_time)
		stats->stop = true;

	last_count = current_count;
	last_stats_time = current_stats_time;
}

void stop_timer() {
	struct itimerval ntimeout;

	signal(SIGALRM, SIG_IGN); /* ignore the timer if it alarms */
	ntimeout.it_interval.tv_sec = ntimeout.it_interval.tv_usec = 0;
	ntimeout.it_value.tv_sec  = ntimeout.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &ntimeout, NULL);	/* stop timer */
}

void stop_handler(int signum) {
	(void)signum;

	stop_timer();
	stats->stop = true;
}

void child_handler(int signum) {
	(void)signum;
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
		if (pid == stats->ping_pid) { /* ping died */
//printf("ping process died\n");
			stats->ping_pid = -1;
			stats->stop = true;

			if (stats->pong_pid != -1)
				kill(stats->pong_pid, SIGINT);
			stop_timer();
		} else if (pid == stats->pong_pid) {
//printf("pong process died\n");
			stats->pong_pid = -1;
			stats->stop = true;

			if (stats->ping_pid != -1)
				kill(stats->ping_pid, SIGINT);
			stop_timer();
		} else if (pid == 0) { /* no more pids have been reported dead */
			break;
		} else {
			printf("Got a SIGCHLD from pid '%d', but...\n", pid);
			printf("\tBilly Jean is not my lover...\n");
			printf("\tshe's just a girl who says that I am the one...\n");
			printf("\tbut, the kid is not my son...\n");
			/* wtf? */
		}
	} /* no more processes have died */
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
int setup_child_signals() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	return 0;
}

void setup_crash_handler() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = print_backtrace2;
	sigaction(SIGILL, &sa, NULL);
}

int do_monitor_work() {
	sigset_t signal_mask;

	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGCHLD);
	sigdelset(&signal_mask, SIGALRM);
	sigdelset(&signal_mask, SIGINT);

	setup_child_signals();
setup_crash_handler();

	setup_timer();
	setup_stop_signal();
	stats->start_time = get_time();
	stats->timeout_time = stats->start_time + config.max_execution_time * 1.0;

//printf("ping pid=%d, pong pid=%d\n", stats->ping_pid, stats->pong_pid);


	while ((stats->stop != true) || (stats->ping_pid != -1) || (stats->pong_pid != -1)) {
		sigsuspend(&signal_mask);
	}

	config.cleanup();
	return 0;
}

/* signal to receive if my parent process dies */
void on_parent_death(int signum) {
	int ret;

	if ((ret = prctl(PR_SET_PDEATHSIG, signum)) == -1) {
		perror("Failed to create message to be delivered upon my becoming an orphan: %m\n");
	}
}

void do_ping_work() {
//	rename_thread("ping-thread");

	on_parent_death(SIGINT);
setup_crash_handler();

	if (config.cpu1 != -1) {
		set_affinity(config.cpu1);
	} else { /* affinity not set */
//		printf ("Affinity not set, however ping *currently* running on cpu %d\n", sched_getcpu());
	}

	while (stats->stop != true) {
		stats->ping_count ++;

		while ((config.do_send(config.mouth[0]) != 1) && (stats->stop != true));
		while ((config.do_recv(config.ear[0]) != 1) && (stats->stop != true));
	}

	/* cleanup */
	config.cleanup();
	exit(0);
}

void do_pong_work() {
//	rename_thread("pong-thread");

	on_parent_death(SIGINT);
setup_crash_handler();

	if (config.cpu2 != -1) {
		set_affinity(config.cpu2);
//		printf("Affinity set for pong to cpu %d\n", config.cpu2);
	} else {
//		printf("Affinity not set, however pong *currently* running on cpu %d\n", sched_getcpu());
	}

	while (stats->stop != true) {
		while ((config.do_recv(config.ear[1]) != 1) && (stats->stop != true));
		while ((config.do_send(config.mouth[1]) != 1) && (stats->stop != true));
	}
	config.cleanup();
	exit(0);
}
int ping_thread_function(void *argument) {
	(void)argument;

	do_ping_work();
	return 0;
}
int pong_thread_function(void *argument) {
	(void)argument;

	do_pong_work();
	return 0;
}
void *pong_pthread_function(void *argument) {
	(void)argument;

	do_pong_work();
	return NULL;
}

int do_fork() {
	pid_t pid;

	pid = fork();
	if (pid == 0) { /* child proc */
		do_pong_work();
	} else if (pid > 0) {
		stats->pong_pid = pid;

		pid = fork();
		if (pid == 0) { /* child proc */
			do_ping_work();
		} else if (pid > 0) {
			stats->ping_pid = pid;

			/* setup child handlers */
			do_monitor_work();

			if (stats->ping_pid > 0)
				kill(stats->ping_pid, SIGINT);
			if (stats->pong_pid > 0)
				kill(stats->pong_pid, SIGINT);
		} else {
			printf("Error while trying to fork: %d: %m\n", stats->ping_pid);
			exit(-1);
		}
	} else {
		printf("Error while trying to fork: %d: %m\n", stats->pong_pid);
		exit(-1);
	}

	return 0;
}
int do_clone() {

//	printf("Min stack size is %d, creating stack with size %d\n", config.min_stack, STACK_SIZE);


	config.stack[0] = calloc(1, STACK_SIZE); /* ping thread */
	config.stack[1] = calloc(1, STACK_SIZE); /* pong thread */

	if ((config.stack[0] == 0) || (config.stack[1] == 0)) {
		perror("calloc: could not allocate stack");
		exit(1);
	}

	stats->ping_pid = clone(&ping_thread_function,
		(char *) config.stack[0] + STACK_SIZE,
		CLONE_FS | CLONE_FILES | SIGCHLD,
		NULL);

/*
	stats->pong_pid = clone(&pong_thread_function,
		(char *) stack + STACK_SIZE,
		SIGCHLD | CLONE_FS | CLONE_FILES | \
		CLONE_SIGHAND | CLONE_VM,
		NULL);
*/
	stats->pong_pid = clone(&pong_thread_function,
		(char *) config.stack[1] + STACK_SIZE,
		CLONE_FS | CLONE_FILES | SIGCHLD,
		NULL);

	if ((stats->ping_pid == -1) || (stats->pong_pid == -1)) {
		perror("clone");
		exit(2);
	}
	do_monitor_work();
//	do_ping_work();
//	free(config.stack[1]);

	return 0;
}

int do_pthread() {
	pthread_t pong_thread;

	pthread_create(&pong_thread, NULL, &pong_pthread_function, NULL);
	do_ping_work();

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

int main(int argc, char *argv[]) {
setup_crash_handler();
	setup_defaults(argv[0]);

	parse_opts(argc, argv);

	do_setup();

	start_threads();


	return 0;
}
