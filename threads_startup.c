#include "threads_startup.h"

#include "threads_monitor.h"
#include "threads_children.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

static int do_fork(void) {
	int thread_num = 0;
	int pid;

	pid = fork();
	if (pid == 0) { /* child proc */ /* ping */
		do_thread_work(thread_num);
	} else if (pid > 0) {
		run_data->thread_info[thread_num].pid = pid;

		thread_num ++;

		pid = fork();
		if (pid == 0) { /* child proc */ /* pong */
			do_thread_work(thread_num);
			exit(0);
		} else if (pid > 0) {
			run_data->thread_info[thread_num].pid = pid;
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
static int do_clone(void) {
	int thread_num = 0;
	int clone_flags;

	run_data->thread_info[0].stack = calloc(1, STACK_SIZE); /* ping thread */
	run_data->thread_info[1].stack = calloc(1, STACK_SIZE); /* pong thread */

	if ((run_data->thread_info[0].stack == 0) || (run_data->thread_info[1].stack == 0)) {
		perror("calloc: could not allocate stack");
		exit(1);
	}

//	clone_flags = CLONE_FS | CLONE_FILES | CLONE_PARENT | CLONE_VM | SIGCHLD;
	clone_flags = CLONE_FS | CLONE_FILES | SIGCHLD;

	run_data->thread_info[thread_num].tid = clone(&thread_function,
		(char *) run_data->thread_info[thread_num].stack + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num],
		&run_data->thread_info[thread_num].ptid, NULL, &run_data->thread_info[thread_num].ctid
		);

	thread_num++;

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

	return 0;
}

static int do_pthread(void) {
	int thread_num = 0;
	pthread_attr_t attr;
	int ret;

	if ((ret = pthread_attr_init(&attr)) != 0) {
		errno = ret; perror("pthread_attr_init"); exit(-1);
	}
	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		errno = ret ; perror("pthread_attr_setdetachstate"); exit(-1);
	}

	if ((ret = pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num])) != 0) {
		errno = ret;
		printf("Error occurred while starting 'ping' thread: %s\n", strerror(errno));
		exit(-1);
	}

	thread_num ++;

	pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num]);


	if ((ret = pthread_attr_destroy(&attr)) != 0) {
		errno = ret; perror("pthread_attr_destroy"); exit(-1);
	}

	return 0;
}

static void init_threads(void) {

	memset(run_data, 0, sizeof(struct run_data_struct));

	run_data->stop = false;
	run_data->rusage_req_in_progress = false;
	run_data->rusage_req[0] = run_data->rusage_req[1] = false;
/*
	memset(&run_data->thread_info[0], 0, sizeof(struct thread_info_struct));
	memset(&run_data->thread_info[1], 0, sizeof(struct thread_info_struct));
	memset((void *)&run_data->thread_stats[0], 0, sizeof(struct thread_stats_struct));
	memset((void *)&run_data->thread_stats[1], 0, sizeof(struct thread_stats_struct));
*/
	run_data->thread_info[0].thread_num = 0;
	run_data->thread_info[1].thread_num = 1;
	strncpy(run_data->thread_info[0].thread_name, "ping_thread", 12);
	strncpy(run_data->thread_info[1].thread_name, "pong_thread", 12);

	run_data->thread_stats[0].sched_data = map_shared_area(PROC_PID_SCHED_SIZE, config.size_align_flag);
	run_data->thread_stats[1].sched_data = map_shared_area(PROC_PID_SCHED_SIZE, config.size_align_flag);

}

int start_threads(void) {
	printf("parent process is %d\n", getpid());

	init_threads();

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

	do_monitor_work();

	return 0;
}
