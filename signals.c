#include "signals.h"

#include "threads_monitor.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <execinfo.h>
#include <sys/wait.h>

long double get_time() {
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return ((double) ts.tv_sec) + ((double)ts.tv_nsec / 1000000000.0);
}

void __attribute__ ((noreturn)) print_backtrace(int signum) {
	void *array[32];	/* Array to store backtrace symbols */
	int size;		/* To store the exact no of values stored */
	char **strings;		/* To store functions from the backtrace list in ARRAY */
	int i;

	printf("print_backtrace: got signal %d\n", signum);

	size = backtrace(array, 32);
	strings = backtrace_symbols(array, size);

	/* prints each string of function names of trace*/
	for (i = 0 ; i < size ; i++)
		fprintf(stderr, "%s\n", strings[i]);

	exit(-1);
}

void __attribute__ ((noreturn)) print_backtrace2(int signum) {
	char buf[1024];

	pid_t dying_pid = getpid();
	pid_t child_pid = fork();

	(void)signum;


	if (child_pid < 0) {
		perror("fork() while collecting backtrace:");
	} else if (child_pid == 0) {
		const char* argv[] = {"sh", "-c", buf, NULL};

		sprintf(buf, "gdb -p %d -batch -ex thread apply all bt 2>/dev/null | "
		"sed '0,/<signal handler/d'", dying_pid);
		execve("/bin/sh", (char**)argv, NULL);
		_exit(1);
	} else {
		waitpid(child_pid, NULL, 0);
	}
	_exit(1);
}

void stop_timer() {
	struct itimerval ntimeout;

	signal(SIGALRM, SIG_IGN); /* ignore the timer if it alarms */
	ntimeout.it_interval.tv_sec = ntimeout.it_interval.tv_usec = 0;
	ntimeout.it_value.tv_sec  = ntimeout.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &ntimeout, NULL);	/* stop timer */
}

int setup_timer() {
	struct sigaction sa;
	struct itimerval timer;

	if (config.stats_interval == 0) /* no updates */
		return 0;

	timer.it_value.tv_sec = config.stats_interval;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = config.stats_interval;
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

	run_data->stop = false;

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


