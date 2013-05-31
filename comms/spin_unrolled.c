#include "spin_unrolled.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int volatile *spin_unrolled_var;
// mem_sync_method = 0 - no memory sync
// mem_sync_method = 1 - mb()
// mem_sync_method = 2 - msync( MS_SYNC )
// mem_sync_method = 3 - msync( MS_INVALIDATE )
// mem_sync_method = 4 - msync( MS_ASYNC )
static int mem_sync_method;

int make_spin_unrolled_pair(int fd[2]) {
	static int spin_unrolled_num = 0;

	if (spin_unrolled_num == 0) {
		spin_unrolled_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*spin_unrolled_var = 0;
		mem_sync_method = 1;
	}

	fd[0] = spin_unrolled_num;
	fd[1] = spin_unrolled_num;

	spin_unrolled_num ++;
	return 0;
}

#define MEM_SYNC_METHOD_0 \

#define MEM_SYNC_METHOD_1 \
	mb();
#define MEM_SYNC_METHOD_2 \
	msync(local_spin_unrolled_var, 2, MS_SYNC);
#define MEM_SYNC_METHOD_3 \
	msync(local_spin_unrolled_var, 3, MS_SYNC);
#define MEM_SYNC_METHOD_4 \
	msync(local_spin_unrolled_var, 4, MS_SYNC);

#define MEM_SYNC_METHOD_NAME_0 "no memory sync"
#define MEM_SYNC_METHOD_NAME_1 "mb()"
#define MEM_SYNC_METHOD_NAME_2 "msync( MS_SYNC )"
#define MEM_SYNC_METHOD_NAME_3 "msync( MS_INVALIDATE )"
#define MEM_SYNC_METHOD_NAME_4 "msync( MS_ASYNC )"

#define do_mem_sync_method(mem_sync_method_val) \
	MEM_SYNC_METHOD_##mem_sync_method_val

#define SET_VAL(val,mem_sync_method_val) \
	do { \
		*spin_unrolled_var = val; \
		do_mem_sync_method(mem_sync_method_val); \
	} while (0)

#define WAIT_VAL(val) \
	while (*spin_unrolled_var != val)

#define ONE_PING(mem_sync_method_val) \
	run_data->ping_count ++; \
	SET_VAL(1,mem_sync_method_val); \
	WAIT_VAL(0)

#define ONE_PONG(mem_sync_method_val) \
	WAIT_VAL(1); \
	SET_VAL(0,mem_sync_method_val)

#define PING_LOOP_METHOD(mem_sync_method_val) \
	PING_LOOP_LABEL_ ## mem_sync_method_val: \
		printf("Pinging with %s\n", MEM_SYNC_METHOD_NAME_ ## mem_sync_method_val); \
		while (1) { \
			ONE_PING(mem_sync_method_val); \
			ONE_PING(mem_sync_method_val); \
			ONE_PING(mem_sync_method_val); \
			ONE_PING(mem_sync_method_val); \
			ONE_PING(mem_sync_method_val); \
		}

#define PONG_LOOP_METHOD(mem_sync_method_val) \
	PONG_LOOP_LABEL_ ## mem_sync_method_val: \
		printf("Ponging with %s\n", MEM_SYNC_METHOD_NAME_ ## mem_sync_method_val); \
		while (1) { \
			ONE_PONG(mem_sync_method_val); \
			ONE_PONG(mem_sync_method_val); \
			ONE_PONG(mem_sync_method_val); \
			ONE_PONG(mem_sync_method_val); \
			ONE_PONG(mem_sync_method_val); \
		}

#pragma GCC push_options
//#pragma GCC optimize("unroll-all-loops --param max-average-unrolled-insns=10000 --param max-unroll-times=100")
//#pragma GCC optimize("unroll-all-loops", "--param max-average-unrolled-insns=10000")
//#pragma GCC optimize("unroll-all-loops,-param max-unroll-times=100")
//#pragma GCC option (@max-average-unrolled-insns{"10000"})

//inline int __PINGPONG_FN __attribute__((optimize("-funroll-all-loops,--param max-unroll-times=3"))) do_ping_spin_unrolled(int thread_num) {
inline int __PINGPONG_FN do_ping_spin_unrolled(int thread_num) {
	void *local_spin_unrolled_var;
	static void *sync_mem_method_table[] = {
		&&PING_LOOP_LABEL_0, &&PING_LOOP_LABEL_1,
		&&PING_LOOP_LABEL_2, &&PING_LOOP_LABEL_3,
		&&PING_LOOP_LABEL_4 };
	(void)thread_num;

	local_spin_unrolled_var = spin_unrolled_var;
	goto *sync_mem_method_table[mem_sync_method];

	PING_LOOP_METHOD(0);
	PING_LOOP_METHOD(1);
	PING_LOOP_METHOD(2);
	PING_LOOP_METHOD(3);
	PING_LOOP_METHOD(4);

}
#pragma GCC pop_options

inline int __PINGPONG_FN do_pong_spin_unrolled(int thread_num) {
	void *local_spin_unrolled_var;
	static void *sync_mem_method_table[] = {
		&&PONG_LOOP_LABEL_0, &&PONG_LOOP_LABEL_1,
		&&PONG_LOOP_LABEL_2, &&PONG_LOOP_LABEL_3,
		&&PONG_LOOP_LABEL_4 };
	(void)thread_num;

	local_spin_unrolled_var = spin_unrolled_var;

	goto *sync_mem_method_table[mem_sync_method];

	PONG_LOOP_METHOD(0);
	PONG_LOOP_METHOD(1);
	PONG_LOOP_METHOD(2);
	PONG_LOOP_METHOD(3);
	PONG_LOOP_METHOD(4);
}

int __CONST cleanup_spin_unrolled(void) {
	munmap((void *)spin_unrolled_var, sizeof(int));
	return 0;
}

static struct comm_mode_ops_struct comm_ops_spin_unrolled = {
	.comm_make_pair		= make_spin_unrolled_pair,
	.comm_do_ping		= do_ping_spin_unrolled,
	.comm_do_pong		= do_pong_spin_unrolled,
	.comm_cleanup		= cleanup_spin_unrolled
};

ADD_COMM_MODE(spin_unrolled, "busy-wait on a shared variable, with extended loops", &comm_ops_spin_unrolled);
