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


#include "sem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>


sem_t *sems[2];
const char *sem_names[] = { "ping", "pong" };
static bool busy = false;

int make_sem_pair(int fd[2]) {
	static int sem_num = 0;
	sem_t *ret;

	if (!strncmp(get_comm_mode_name(config.comm_mode_index), "busysem", 7)) {
		busy = true;
	}


	if ((ret = sem_open(sem_names[sem_num], O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		printf("Error opening semaphore '%s': %s\n", sem_names[sem_num], strerror(errno));
		exit(-1);
	}

	fd[0] = fd[1] = sem_num;
	sems[sem_num] = ret;

//	sems[1] = sem_open(config.sem_names[1], O_CREAT, S_IRUSR | S_IWUSR, 0);

	sem_num ++;
	return 0;
}

int do_begin_sem(void) {
	return sem_post(sems[0]);
}

inline void __PINGPONG_FN comm_ping_sem(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		while (sem_wait(sems[0]) != 0);
		while (sem_post(sems[1]) != 0);
	}
}

inline void __PINGPONG_FN comm_pong_sem(int thread_num) {
	(void)thread_num;

	while (1) {
		while (sem_wait(sems[1]) != 0);
		while (sem_post(sems[0]) != 0);
	}
}

inline void __PINGPONG_FN comm_ping_busysem(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		while (sem_trywait(sems[0]) == -1);
		while (sem_post(sems[1]) != 0);
	}
}

inline void __PINGPONG_FN comm_pong_busysem(int thread_num) {
	(void)thread_num;

	while (1) {
		while (sem_trywait(sems[1]) == -1);
		while (sem_post(sems[0]) != 0);
	}
}

int cleanup_sem(void) {
	sem_close(sems[0]);
	sem_unlink(sem_names[0]);
	sem_close(sems[1]);
	sem_unlink(sem_names[1]);

	return 0;
}

static struct comm_mode_ops_struct comm_ops_sem = {
	.comm_make_pair		= make_sem_pair,
	.comm_begin		= do_begin_sem,
	.comm_ping		= comm_ping_sem,
	.comm_pong		= comm_pong_sem,
	.comm_cleanup		= cleanup_sem
};

static struct comm_mode_ops_struct comm_ops_busysem = {
	.comm_make_pair		= make_sem_pair,
	.comm_begin		= do_begin_sem,
	.comm_ping		= comm_ping_busysem,
	.comm_pong		= comm_pong_busysem,
	.comm_cleanup		= cleanup_sem
};

ADD_COMM_MODE(sem, "wait on a semaphore", &comm_ops_sem);
ADD_COMM_MODE(busysem, "busy-wait on a semaphore", &comm_ops_busysem);
