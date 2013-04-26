
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "threads_monitor.h"
#include "threads_children.h"
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


struct interval_stats_struct {
	unsigned long long current_count;
	unsigned long long interval_count;
	long double current_time;
	long double interval_time;
	long double run_time;
	struct rusage rusage[2];
	unsigned long long interval_tsc[2];

	long csw[2]; /* per-thread per-interval csw */

	long double iteration_time; /* time for each iteration */
	long double cpi[2]; /* cycles-per-iteration per CPU */
	long double mhz[2]; /* approximate speed of the CPU */
};

static void monitor_cleanup() {
	free(run_data->thread_info[0].stack);
	free(run_data->thread_info[1].stack);

	munmap(run_data, sizeof(struct run_data_struct));
}


void stop_handler(int signum) {
	static int visited = 0;
	(void)signum;

	if (visited) {
		printf("Trying to stop a second time?\n");
		return;
	}
	visited++;

	stop_timer();
	run_data->stop = true;

	send_sig(run_data->thread_info[0].pid, SIGUSR1);
	send_sig(run_data->thread_info[1].pid, SIGUSR2);

	printf("Waiting for child threads to die.  Die, kids!  Die!!!\n");
	while ((run_data->thread_info[0].pid != -1) || (run_data->thread_info[1].pid != -1)) {
		do_sleep(0, 5000000);
	}

	config.comm_cleanup();

	monitor_cleanup();

	exit(0);
}



static int gather_stats(struct interval_stats_struct *i_stats) {

	i_stats->current_count = run_data->ping_count;
	i_stats->current_time = get_time();

	run_data->rusage_req_in_progress = true;
	run_data->rusage_req[0] = true;
	run_data->rusage_req[1] = true;
	send_sig(run_data->thread_info[0].pid, SIGUSR1);
	send_sig(run_data->thread_info[1].pid, SIGUSR2);

	i_stats->run_time = i_stats->current_time - run_data->start_time;
	i_stats->interval_time = i_stats->current_time - run_data->last_stats_time;
	i_stats->interval_count = i_stats->current_count - run_data->last_ping_count;


	while (run_data->rusage_req_in_progress == true) {
		if ((run_data->rusage_req[0] == false) && (run_data->rusage_req[1] == false))
			run_data->rusage_req_in_progress = false;
		if (run_data->rusage_req_in_progress == true)
			__sync_synchronize();
		if (run_data->stop == true)
			return -1;
	}

	i_stats->rusage[0].ru_nvcsw = run_data->thread_stats[0].rusage.ru_nvcsw - run_data->thread_stats[0].last_rusage.ru_nvcsw;
	i_stats->rusage[0].ru_nivcsw = run_data->thread_stats[0].rusage.ru_nivcsw - run_data->thread_stats[0].last_rusage.ru_nivcsw;
	i_stats->rusage[1].ru_nvcsw = run_data->thread_stats[1].rusage.ru_nvcsw - run_data->thread_stats[1].last_rusage.ru_nvcsw;
	i_stats->rusage[1].ru_nivcsw = run_data->thread_stats[1].rusage.ru_nivcsw - run_data->thread_stats[1].last_rusage.ru_nivcsw;

	i_stats->csw[0] = i_stats->rusage[0].ru_nvcsw + i_stats->rusage[0].ru_nivcsw;
	i_stats->csw[1] = i_stats->rusage[1].ru_nvcsw + i_stats->rusage[1].ru_nivcsw;


	if (config.set_affinity == true) {
		i_stats->interval_tsc[0] = run_data->thread_stats[0].tsc - run_data->thread_stats[0].last_tsc;
		i_stats->interval_tsc[1] = run_data->thread_stats[1].tsc - run_data->thread_stats[1].last_tsc;
		i_stats->mhz[0] = i_stats->interval_tsc[0] / i_stats->interval_time / 1000 / 1000;
		i_stats->mhz[1] = i_stats->interval_tsc[1] / i_stats->interval_time / 1000 / 1000;

		i_stats->cpi[0] = (long double)i_stats->interval_tsc[0] / (long double)i_stats->interval_count;
		i_stats->cpi[1] = (long double)i_stats->interval_tsc[1] / (long double)i_stats->interval_count;
	} else {
		i_stats->interval_tsc[0] = i_stats->interval_tsc[1] = 0;
		i_stats->mhz[0] = i_stats->mhz[1] = 0;
		i_stats->cpi[0] = i_stats->cpi[1] = 0;
	}

	i_stats->iteration_time = i_stats->interval_time / i_stats->interval_count;

	return 0;
}

void show_stats(int signum) {
	static char output_buffer[400];
	static char temp_string1[30];
	static char temp_string2[30];

	struct interval_stats_struct i_stats;

	(void)signum;

	memset(&i_stats, 0, sizeof(struct interval_stats_struct));
	memset(output_buffer, 0, 400);
	memset(temp_string1, 0, 30);
	memset(temp_string2, 0, 30);

	if (run_data->rusage_req_in_progress == true) /* already waiting for results */
		return;
	gather_stats(&i_stats);

	if (run_data->stop == true) /* don't worry about output...  let's just pack up and go home */
		return;

	snprintf(output_buffer, 255, "%s - %llu iterations -> %s",
		subsec_string(temp_string1, i_stats.run_time, 3),
		i_stats.interval_count,
		subsec_string(temp_string2, i_stats.iteration_time, 3));

	output_buffer[255] = 0;
	write(1, output_buffer, strlen(output_buffer));

	if (config.set_affinity == true) {
		snprintf(output_buffer, 255, ", ping: %d.%d cycles, pong: %d.%d cycles",
			(int)i_stats.cpi[0], (int)(i_stats.cpi[0] * 100.0) % 100,
			(int)i_stats.cpi[1], (int)(i_stats.cpi[1] * 100.0) & 100);
		write(1, output_buffer, strlen(output_buffer));
	}

	// for now, let's ignore the context switches (i_stats.csw[ * ]) and calculated mhz (i_stats.mhz[ * ])

	write(1, "\n", 1);

	/* cleanup things for the next time we come back */
	memcpy((void *)&run_data->thread_stats[0].last_rusage, (void *)&run_data->thread_stats[0].rusage, sizeof(struct rusage));
	memcpy((void *)&run_data->thread_stats[1].last_rusage, (void *)&run_data->thread_stats[1].rusage, sizeof(struct rusage));

	run_data->last_ping_count = i_stats.current_count;
	run_data->last_stats_time = i_stats.current_time;
	run_data->thread_stats[0].last_tsc = run_data->thread_stats[0].tsc;
	run_data->thread_stats[1].last_tsc = run_data->thread_stats[1].tsc;

	if (i_stats.current_time >= run_data->timeout_time) {
		stop_handler(0);
		return;
	}
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

int do_monitor_work() {
	sigset_t signal_mask;


	setup_child_signals();
	setup_crash_handler();
	setup_stop_signal();

	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGCHLD);
	sigdelset(&signal_mask, SIGALRM);
	sigdelset(&signal_mask, SIGINT);

	while ((run_data->ready[0] != true) || (run_data->ready[1] != true)) {
		do_sleep(0, 5000000);
	}

	setup_timer();
	__sync_synchronize();
	run_data->start = true; /* tell the child threads to begin */
	config.comm_begin();
	__sync_synchronize();

	run_data->start_time = run_data->last_stats_time = get_time();
	run_data->timeout_time = (double)run_data->start_time + (double)config.runtime;

	while (run_data->stop != true) {
		sigsuspend(&signal_mask);
	}
	config.comm_interrupt(SIGINT);
	stop_handler(1);

	return 0;
}

static int do_fork() {
	int thread_num = 0;
	int pid;

	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "ping_thread", 12);
	run_data->stop = false;
	run_data->rusage_req_in_progress = false;
	run_data->rusage_req[0] = run_data->rusage_req[1] = false;

	pid = fork();
	if (pid == 0) { /* child proc */ /* ping */
		do_thread_work(thread_num);
	} else if (pid > 0) {
		run_data->thread_info[thread_num].pid = pid;

		thread_num ++;

		run_data->thread_info[thread_num].thread_num = thread_num;
		strncpy(run_data->thread_info[thread_num].thread_name, "pong_thread", 12);

		pid = fork();
		if (pid == 0) { /* child proc */ /* pong */
			do_thread_work(thread_num);
		} else if (pid > 0) {
			run_data->thread_info[thread_num].pid = pid;

			do_monitor_work();
		} else {
			printf("Error while trying to fork: %d: %s\n",
				run_data->thread_info[thread_num].pid, strerror(errno));
			exit(-1);
		}
	} else {
		printf("Error while trying to fork: %d: %s\n",
			run_data->thread_info[thread_num].pid, strerror(errno));
		exit(-1);
	}

	return 0;
}
static int do_clone() {
	int thread_num = 0;
	int clone_flags;

	run_data->thread_info[0].stack = calloc(1, STACK_SIZE); /* ping thread */
	run_data->thread_info[1].stack = calloc(1, STACK_SIZE); /* pong thread */

	if ((run_data->thread_info[0].stack == 0) || (run_data->thread_info[1].stack == 0)) {
		perror("calloc: could not allocate stack");
		exit(1);
	}


	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "ping_thread", 12);

//	clone_flags = CLONE_FS | CLONE_FILES | CLONE_PARENT | CLONE_VM | SIGCHLD;
	clone_flags = CLONE_FS | CLONE_FILES | SIGCHLD;

	run_data->thread_info[thread_num].tid = clone(&thread_function,
		(char *) run_data->thread_info[thread_num].stack + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num],
		&run_data->thread_info[thread_num].ptid, NULL, &run_data->thread_info[thread_num].ctid
		);

	thread_num++;


	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "pong_thread", 12);

	run_data->thread_info[thread_num].tid = clone(&thread_function,
		(char *) run_data->thread_info[thread_num].stack + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num],
		&run_data->thread_info[thread_num].ptid, NULL, &run_data->thread_info[thread_num].ctid
		);
	/* what about CLONE_SIGHAND and CLONE_VM ? */


	if ((run_data->thread_info[0].pid == -1) || (run_data->thread_info[1].pid == -1)) {
		perror("clone");
		exit(2);
	}
	do_monitor_work();

	return 0;
}

static int do_pthread() {
	int thread_num = 0;
	pthread_attr_t attr;
	int ret;

	if ((ret = pthread_attr_init(&attr)) != 0) {
		errno = ret; perror("pthread_attr_init"); exit(-1);
	}
	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		errno = ret ; perror("pthread_attr_setdetachstate"); exit(-1);
	}

	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "ping_thread", 12);

	if ((ret = pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num])) != 0) {
		errno = ret;
		printf("Error occurred while starting 'ping' thread: %s\n", strerror(errno));
		exit(-1);
	}

	thread_num ++;

	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "pong_thread", 12);

	pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num]);


	if ((ret = pthread_attr_destroy(&attr)) != 0) {
		errno = ret; perror("pthread_attr_destroy"); exit(-1);
	}

	do_monitor_work();

	return 0;
}

int start_threads() {
	printf("parent process is %d\n", getpid());

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
