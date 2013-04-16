#include "spin.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>

int volatile *spin_var;

int make_spin_pair(int fd[2]) {
	static int spin_num = 0;

	if (spin_num == 0) {
		spin_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*spin_var = 0;
	}

	fd[0] = spin_num;
	fd[1] = spin_num;

	spin_num ++;
	return 0;
}

inline int __PINGPONG_FN do_ping_spin(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		do {
			*spin_var = 1;
//			__sync_synchronize();
		} while (0);
		while (*spin_var != 0) {
//			__sync_synchronize();
		}
	}
}

inline int __PINGPONG_FN do_pong_spin(int thread_num) {
	(void)thread_num;
	while (1) {

		while (*spin_var != 1) {
//			__sync_synchronize();
		}
		do {
			*spin_var = 0;
//			__sync_synchronize();
		} while (0);
	}
}

int cleanup_spin() {

	return 0;
}

void __attribute__((constructor)) comm_add_spin() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = "spin";

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_spin_pair;
	ops.comm_do_ping = do_ping_spin;
	ops.comm_do_pong = do_pong_spin;
	ops.comm_cleanup = cleanup_spin;

	comm_mode_do_initialization(&init_info, &ops);

}
ADD_COMM_MODE(spin, comm_add_spin);
