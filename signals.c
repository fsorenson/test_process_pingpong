#include "signals.h"

#include "threads_monitor.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <execinfo.h>
#include <sys/wait.h>



void print_backtrace(int signum) {
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
}

void __NORETURN print_backtrace_die(int signum) {
	print_backtrace(signum);

	exit(-1);
}

void __NORETURN print_backtrace2(int signum) {
	char buf[1024];

	pid_t dying_pid = getpid();
	pid_t child_pid = fork();

	(void)signum;


	if (child_pid < 0) {
		perror("fork() while collecting backtrace:");
	} else if (child_pid == 0) {
		const char* argv[] = {"sh", "-c", buf, NULL};

		snprintf(buf, 1024, "gdb -p %d -batch -ex thread apply all bt 2>/dev/null | "
		"sed '0,/<signal handler/d'", dying_pid);
		execve("/bin/sh", (char**)argv, NULL);
		_exit(1);
	} else {
		waitpid(child_pid, NULL, 0);
	}
	_exit(1);
}

void stop_timer(void) {
	struct itimerval ntimeout;

	signal(SIGALRM, SIG_IGN); /* ignore the timer if it alarms */
	ntimeout.it_interval.tv_sec = ntimeout.it_interval.tv_usec = 0;
	ntimeout.it_value.tv_sec  = ntimeout.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &ntimeout, NULL);	/* stop timer */
}

void setup_timer(void) {
	struct sigaction sa;
	struct itimerval timer;

	if (config.stats_interval == 0) /* no updates */
		return;

	timer.it_value.tv_sec = config.stats_interval;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = config.stats_interval;
	timer.it_interval.tv_usec = 0;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &show_stats;

	sigaction(SIGALRM, &sa, NULL);
	setitimer(ITIMER_REAL, &timer, 0);

	return;
}
void setup_stop_signal(void) {
	struct sigaction sa;

	run_data->stop = false;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &stop_handler;
	sigaction(SIGINT, &sa, NULL);

	return;
}
void setup_child_signals(void) {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	return;
}

void setup_crash_handler(void) {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = print_backtrace2;
	sigaction(SIGILL, &sa, NULL);
}

/* slightly safer version of kill...  only send to real pids, not:
 *  0    - process group id
 *  -1   - all processes for which we have permission to send the signal
 *  < -2 - all processes whose process group id is equal to the absolute value
 *         of pid, for which we have permission to send a signal
 *
 * just call this...  it's more predictable
 */
int send_sig(int pid, int sig) {
	if (pid > 0)
		return kill(pid, sig);

	return 0;
}

