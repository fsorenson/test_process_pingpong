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

int do_begin_sem() {
	return sem_post(sems[0]);
}

inline int __attribute__((noreturn)) __attribute__((hot)) __attribute__((optimize("-Ofast"))) do_ping_sem(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		while (sem_wait(sems[0]) != 0);
		while (sem_post(sems[1]) != 0);
	}
}

inline int __attribute__((noreturn)) __attribute__((hot)) __attribute__((optimize("-Ofast"))) do_pong_sem(int thread_num) {
	(void)thread_num;

	while (1) {
		while (sem_wait(sems[1]) != 0);
		while (sem_post(sems[0]) != 0);
	}
}

inline int __attribute__((hot)) __attribute__((optimize("-Ofast"))) do_ping_busysem(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		while (sem_trywait(sems[0]) == -1);
		while (sem_post(sems[1]) != 0);
	}
}

inline int __attribute__((hot)) __attribute__((optimize("-Ofast"))) do_pong_busysem(int thread_num) {
	(void)thread_num;

	while (1) {
		while (sem_trywait(sems[1]) == -1);
		while (sem_post(sems[0]) != 0);
	}
}


inline int do_send_sem(int fd) {

        return ! sem_post(sems[fd]);
}
inline int do_recv_sem(int fd) {
        int err;

	err = sem_wait(sems[fd]);
        return ! err;
}
inline int do_recv_busy_sem(int fd) {
        int err;

       do {
		if ((err = sem_trywait(sems[fd])) == -1)
			err = errno;
	} while (err == EAGAIN);

        return ! err;
}
int cleanup_sem() {
	sem_close(sems[0]);
	sem_unlink(sem_names[0]);
	sem_close(sems[1]);
	sem_unlink(sem_names[1]);

	return 0;
}

void __attribute__((constructor)) comm_add_sem() {
	struct comm_mode_ops_struct ops;

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_sem_pair;
	ops.comm_begin = do_begin_sem;
	ops.comm_do_ping = do_ping_sem;
	ops.comm_do_pong = do_pong_sem;
	ops.comm_do_send = do_send_sem;
	ops.comm_do_recv = do_recv_sem;
	ops.comm_cleanup = cleanup_sem;


	comm_mode_do_initialization("sem", &ops);
}

void __attribute__((constructor)) comm_add_busy_sem() {
	struct comm_mode_ops_struct ops;

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_sem_pair;
	ops.comm_begin = do_begin_sem;
	ops.comm_do_ping = do_ping_busysem;
	ops.comm_do_pong = do_pong_busysem;
	ops.comm_do_send = do_send_sem;
	ops.comm_do_recv = do_recv_busy_sem;
	ops.comm_cleanup = cleanup_sem;


	comm_mode_do_initialization("busysem", &ops);
}

ADD_COMM_MODE(sem, comm_add_sem);
ADD_COMM_MODE(busysem, comm_add_busysem);

