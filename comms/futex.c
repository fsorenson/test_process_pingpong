#include "futex.h"
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

static int shm_id;
int *futex_id[2];
int futex_vals[2];

int make_futex_pair(int fd[2]) {
	static int futex_num = 0;

	if (futex_num == 0) {
		shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
	}

	futex_id[futex_num] = shmat(shm_id, NULL, 0);
	futex_vals[futex_num] = 0x0A + futex_num;
	*futex_id[futex_num] = futex_vals[0];
	fd[0] = futex_num;
	fd[1] = futex_num ^ 0x01;

	futex_num ++;
	return 0;
}

inline int do_send_futex(int fd) {
	*futex_id[fd] = futex_vals[fd];

	while ((! syscall(SYS_futex, futex_id[fd], FUTEX_WAKE, 1, NULL, NULL, NULL)) && (run_data->stop != true)) {
		sched_yield();
	}
	return 1;
}
inline int do_recv_futex(int fd) {
	while ((syscall(SYS_futex, futex_id[fd], FUTEX_WAIT, futex_vals[fd ^ 1], NULL, NULL, NULL))
		&& (run_data->stop != true)) {
		sched_yield();
	}
	return 1;
}


int cleanup_futex(void) {
	shmctl(shm_id, IPC_RMID, 0);

	return 0;
}

static struct comm_mode_ops_struct comm_ops_futex = {
	.comm_make_pair = make_futex_pair,
	.comm_do_send = do_send_futex,
	.comm_do_recv = do_recv_futex,
	.comm_cleanup = cleanup_futex
};

ADD_COMM_MODE(futex, "use futex wait/wake to benefit from kernel-arbitrated lock contention", &comm_ops_futex);
