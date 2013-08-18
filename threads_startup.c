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
		} else
			exit_fail("Error while trying to fork: %d: %s\n",
				run_data->thread_info[thread_num].pid, strerror(errno));
	} else
		exit_fail("Error while trying to fork: %d: %s\n",
			run_data->thread_info[thread_num].pid, strerror(errno));

	return 0;
}
static int do_clone(void) {
	int thread_num = 0;
	int clone_flags;

	run_data->thread_info[0].stack = calloc(1, STACK_SIZE); /* ping thread */
	run_data->thread_info[1].stack = calloc(1, STACK_SIZE); /* pong thread */

	if ((run_data->thread_info[0].stack == 0) || (run_data->thread_info[1].stack == 0))
		exit_fail("calloc: could not allocate stack: %s\n", strerror(errno));

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


	if ((run_data->thread_info[0].pid == -1) || (run_data->thread_info[1].pid == -1))
		exit_fail("clone: could not clone child thread: %s\n", strerror(errno));

	return 0;
}

static int do_pthread(void) {
	int thread_num = 0;
	pthread_attr_t attr;
	int ret;

	if ((ret = pthread_attr_init(&attr)) != 0)
		exit_fail("pthread_attr_init: error: %s\n", strerror(ret));
	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
		exit_fail("pthread_attr_setdetachstate: error: %s\n", strerror(ret));

	if ((ret = pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num])) != 0)
		exit_fail("Error occurred while starting 'ping' thread: %s\n", strerror(ret));

	thread_num ++;

	pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num]);


	if ((ret = pthread_attr_destroy(&attr)) != 0)
		exit_fail("pthread_attr_destroy: error: %s\n", strerror(ret));

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
	} else
		exit_fail("No thread mode specified?\n");

	do_monitor_work();

	return 0;
}
