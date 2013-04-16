#include "futex.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <string.h>

#include <linux/futex.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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


int cleanup_futex() {
	shmctl(shm_id, IPC_RMID, 0);

	return 0;
}

void __attribute__((constructor)) comm_add_futex() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = "futex";
	init_info.help_text = "use futex wait/wake to benefit from kernel-arbitrated lock contention";

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_futex_pair;
	ops.comm_do_send = do_send_futex;
	ops.comm_do_recv = do_recv_futex;
	ops.comm_cleanup = cleanup_futex;

	comm_mode_do_initialization(&init_info, &ops);
}

ADD_COMM_MODE(futex, comm_add_futex);
