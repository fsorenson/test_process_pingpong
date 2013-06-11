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


#include "futex_test.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <string.h>

#include <linux/futex.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#ifndef SYS_futex
#define SYS_futex __NR_futex
#endif

static int shm_id;
static int *futex_value;
static int *futex_id[2];
static int futex_vals[2];

int make_futex_test_pair(int fd[2]) {
	static int futex_num = 0;

	if (futex_num == 0) {
		futex_value = mmap(NULL, sizeof(int),
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*futex_value = 0;

		shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
	}

	fd[0] = futex_num;
	fd[1] = futex_num ^ 0x01;

	futex_num ++;
	return 0;
}

inline int __PINGPONG_FN do_ping_futex_test(int thread_num) {

	while (1) {
		run_data->ping_count ++;
printf("Thread %d, A, value=%d\n", thread_num, *futex_value);

//		while (syscall(SYS_futex, futex_value, FUTEX_WAKE, thread_num, NULL, NULL, NULL) != 1) {
		while (!(syscall(SYS_futex, futex_value, FUTEX_WAKE, thread_num, NULL, NULL, NULL)) != 1) {
		}
printf("Thread %d, B, value=%d\n", thread_num, *futex_value);
//		while (syscall(SYS_futex, futex_value, FUTEX_WAIT, thread_num ^ 1, NULL, NULL, NULL) != 1) {
		while (syscall(SYS_futex, futex_value, FUTEX_WAIT, thread_num ^ 1, NULL, NULL, NULL) != 1) {
		}
	}

}

inline int __PINGPONG_FN do_pong_futex_test(int thread_num) {

	while (1) {

		while (syscall(SYS_futex, futex_value, FUTEX_WAIT, thread_num ^ 1, NULL, NULL, NULL) != 1) {
		}

		while (syscall(SYS_futex, futex_value, FUTEX_WAIT, thread_num, NULL, NULL, NULL) != 1) {
		}
	}
}


inline int do_send_futex_test(int fd) {
	*futex_id[fd] = futex_vals[fd];

	printf("send %d\n", fd);
	while (! syscall(SYS_futex, futex_id[fd], FUTEX_WAKE, 1, NULL, NULL, NULL)) {
		sched_yield();
	}
	return 1;
}
inline int do_recv_futex_test(int fd) {
+printf("recv %d, value=%d\n", fd, *futex_value);
	while (syscall(SYS_futex, futex_id[fd], FUTEX_WAIT, futex_vals[fd ^ 1], NULL, NULL, NULL)) {
//	while ((syscall(SYS_futex, futex_value, FUTEX_WAIT, 0, NULL, NULL, NULL))
//	while (syscall(SYS_futex, futex_value, FUTEX_WAIT, 1, NULL, NULL, NULL) != 1) {
		sched_yield();
	}
	return 1;
}


int cleanup_futex_test(void) {
	shmctl(shm_id, IPC_RMID, 0);

	return 0;
}

static struct comm_mode_ops_struct comm_ops_futex_test = {
	.comm_make_pair = make_futex_test_pair,
	.comm_do_ping = do_ping_futex_test,
	.comm_do_pong = do_pong_futex_test,
	.comm_do_send = do_send_futex_test,
	.comm_do_recv = do_recv_futex_test
};

ADD_COMM_MODE(futex_test, "use futex wait/wake to benefit from kernel-arbitrated lock contention", &comm_ops_futex_test);
