#include "threads_monitor.h"

#include "threads_children.h"
#include "stats_periodic.h"
#include "stats_final.h"
#include "setup.h"
#include "comms.h"


#include "units.h"
#include "sched.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/mman.h>



/* thread startup, execution, handlers, etc */

static inline unsigned long long native_read_tscp(unsigned int *aux)
{
        unsigned long low, high;
        asm volatile(".byte 0x0f,0x01,0xf9"
                     : "=a" (low), "=d" (high), "=c" (*aux));
        return low | ((unsigned long long)high << 32);
}


static void monitor_cleanup(void) {
	free(run_data->thread_info[0].stack);
	free(run_data->thread_info[1].stack);

	munmap(run_data, sizeof(struct run_data_struct));
}

static void stop_threads(void) {
	run_data->stop = true;
	mb();

	send_sig(run_data->thread_info[0].pid, SIGUSR1);
	send_sig(run_data->thread_info[1].pid, SIGUSR2);
}

static void stop_timer(void) {
	struct itimerval ntimeout;

	signal(SIGALRM, SIG_IGN); /* ignore the timer if it alarms */
	ntimeout.it_interval.tv_sec = ntimeout.it_interval.tv_usec = 0;
	ntimeout.it_value.tv_sec  = ntimeout.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &ntimeout, NULL);        /* stop timer */
}

static void stop_handler(int signum) {
	static int visited = 0;
	(void)signum;

	if (visited) {
		printf("Trying to stop a second time?\n");
		return;
	}
	visited++;

	stop_timer();
	stop_threads();

	printf("Waiting for child threads to die.  Die, kids!  Die!!!\n");
	while ((run_data->thread_info[0].pid != -1) || (run_data->thread_info[1].pid != -1)) {
		do_sleep(0, 5000000);
	}

	config.comm_cleanup();

	output_final_stats();

	monitor_cleanup();

	exit(0);
}

static void setup_stop_handler(void) {
	struct sigaction sa;

	run_data->stop = false;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &stop_handler;
	sigaction(SIGINT, &sa, NULL);
}


static void monitor_interrupt(int signum) {
	(void)signum;

	show_periodic_stats();

	if (run_data->last_stats_time >= run_data->timeout_time) {
		stop_handler(0);
		return;
	}

}

static void setup_monitor_timer(void) {
	struct sigaction sa;
	struct itimerval timer;

	if ((config.stats_interval.tv_sec == 0) && (config.stats_interval.tv_usec == 0)) /* no updates */
		return;

	timer.it_value.tv_sec = config.stats_interval.tv_sec;
	timer.it_value.tv_usec = config.stats_interval.tv_usec;
	timer.it_interval.tv_sec = config.stats_interval.tv_sec;
	timer.it_interval.tv_usec = config.stats_interval.tv_usec;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &monitor_interrupt;

	sigaction(SIGALRM, &sa, NULL);
	setitimer(ITIMER_REAL, &timer, 0);

	return;
}

void child_handler(int signum) {
	pid_t pid;
	int status;

	(void)signum;


	while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
		if (pid == run_data->thread_info[0].pid) {
			run_data->thread_info[0].pid = -1;
			run_data->stop = true;

			stop_timer();
		} else if (pid == run_data->thread_info[1].pid) {
			run_data->thread_info[1].pid = -1;
			run_data->stop = true;

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

int do_monitor_work(void) {
	sigset_t signal_mask;


	setup_child_signals();
	setup_crash_handler();
	setup_stop_handler();

	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGCHLD);
	sigdelset(&signal_mask, SIGALRM);
	sigdelset(&signal_mask, SIGINT);

	while ((run_data->ready[0] != true) || (run_data->ready[1] != true)) {
		do_sleep(0, 5000000);
	}

	setup_monitor_timer();
	mb();
	run_data->start = true; /* tell the child threads to begin */
	mb();
	config.comm_begin();

	run_data->start_time = run_data->last_stats_time = get_time();
	run_data->timeout_time = (double)run_data->start_time + (double)config.runtime;

	while (run_data->stop != true) {
		sigsuspend(&signal_mask);
	}
	stop_handler(1);

	return 0;
}
