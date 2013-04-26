#include "sem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>

char comm_name_sem[] = "sem";
char comm_help_text_sem[] =  "wait on a semaphore to pingpong";

char comm_name_busysem[] = "busysem";
char comm_help_text_busysem[] = "busy-wait on a semaphore";

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

int do_begin_sem() {
	return sem_post(sems[0]);
}

inline int __PINGPONG_FN do_ping_sem(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		while (sem_wait(sems[0]) != 0);
		while (sem_post(sems[1]) != 0);
	}
}

inline int __PINGPONG_FN do_pong_sem(int thread_num) {
	(void)thread_num;

	while (1) {
		while (sem_wait(sems[1]) != 0);
		while (sem_post(sems[0]) != 0);
	}
}

inline int __PINGPONG_FN do_ping_busysem(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		while (sem_trywait(sems[0]) == -1);
		while (sem_post(sems[1]) != 0);
	}
}

inline int __PINGPONG_FN do_pong_busysem(int thread_num) {
	(void)thread_num;

	while (1) {
		while (sem_trywait(sems[1]) == -1);
		while (sem_post(sems[0]) != 0);
	}
}

int cleanup_sem() {
	sem_close(sems[0]);
	sem_unlink(sem_names[0]);
	sem_close(sems[1]);
	sem_unlink(sem_names[1]);

	return 0;
}

static struct comm_mode_init_info_struct comm_info_sem = {
	.name = comm_name_sem,
	.help_text = comm_help_text_sem
};

static struct comm_mode_ops_struct comm_ops_sem = {
	.comm_make_pair = make_sem_pair,
	.comm_begin = do_begin_sem,
	.comm_do_ping = do_ping_sem,
	.comm_do_pong = do_pong_sem,
	.comm_cleanup = cleanup_sem
};

void __attribute__((constructor)) comm_add_sem() {
	comm_mode_do_initialization(&comm_info_sem, &comm_ops_sem);
}


static struct comm_mode_init_info_struct comm_info_busysem = {
	.name = comm_name_busysem,
	.help_text = comm_help_text_busysem
};

static struct comm_mode_ops_struct comm_ops_busysem = {
	.comm_make_pair = make_sem_pair,
	.comm_begin = do_begin_sem,
	.comm_do_ping = do_ping_busysem,
	.comm_do_pong = do_pong_busysem,
	.comm_cleanup = cleanup_sem
};

void __attribute__((constructor)) comm_add_busy_sem() {
	comm_mode_do_initialization(&comm_info_busysem, &comm_ops_busysem);
}

ADD_COMM_MODE(sem, comm_add_sem);
ADD_COMM_MODE(busysem, comm_add_busysem);

