#include "spin.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int volatile *spin_var;
static int mem_sync_method;
// mem_sync_method = 0 - mb()
// mem_sync_method = 1 - msync( MS_SYNC )
// mem_sync_method = 2 - msync( MS_INVALIDATE )
// mem_sync_method = 3 - msync( MS_ASYNC )

int make_spin_pair(int fd[2]) {
	static int spin_num = 0;

	if (spin_num == 0) {
		spin_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*spin_var = 0;
		mem_sync_method = 0;
	}

	fd[0] = spin_num;
	fd[1] = spin_num;

	spin_num ++;
	return 0;
}


#define MEM_SYNC_METHOD_0 \
	mb()
#define MEM_SYNC_METHOD_1 \
	msync(local_spin_var, 1, MS_SYNC)
#define MEM_SYNC_METHOD_2 \
	msync(local_spin_var, 2, MS_SYNC)
#define MEM_SYNC_METHOD_3 \
	msync(local_spin_var, 3, MS_SYNC)

#define MEM_SYNC_METHOD_NAME_0 "mb()"
#define MEM_SYNC_METHOD_NAME_1 "msync( MS_SYNC )"
#define MEM_SYNC_METHOD_NAME_2 "msync( MS_INVALIDATE )"
#define MEM_SYNC_METHOD_NAME_3 "msync( MS_ASYNC )"

#define do_mem_sync_method(val) \
	MEM_SYNC_METHOD_##val

#define LOOP_METHOD(val) \
	LOOP_LABEL_ ## val: \
		printf("Pinging with %s\n", MEM_SYNC_METHOD_NAME_ ## val); \
		while (1) { \
			run_data->ping_count ++; \
\
			do { \
				*spin_var = 1; \
				do_mem_sync_method(val); \
			} while (0); \
			while (*spin_var != 0) { \
			} \
\
\
		}

inline int __PINGPONG_FN do_ping_spin(int thread_num) {
	void *local_spin_var;
	static void *sync_mem_method_table[] = {
		&&LOOP_LABEL_0, &&LOOP_LABEL_1,
		&&LOOP_LABEL_2, &&LOOP_LABEL_3 };
	(void)thread_num;

	local_spin_var = spin_var;

	goto *sync_mem_method_table[mem_sync_method];

	LOOP_METHOD(0);
	LOOP_METHOD(1);
	LOOP_METHOD(2);
	LOOP_METHOD(3);

}

inline int __PINGPONG_FN do_pong_spin(int thread_num) {
	(void)thread_num;

	while (1) {

		while (*spin_var != 1) {
		}
		do {
			*spin_var = 0;
			mb();
		} while (0);
	}
}

int __CONST cleanup_spin(void) {
	munmap((void *)spin_var, sizeof(int));
	return 0;
}

static struct comm_mode_ops_struct comm_ops_spin = {
	.comm_make_pair		= make_spin_pair,
	.comm_do_ping		= do_ping_spin,
	.comm_do_pong		= do_pong_spin,
	.comm_cleanup		= cleanup_spin
};

ADD_COMM_MODE(spin, "busy-wait on a shared variable", &comm_ops_spin);
