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


#include "spin.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int volatile *spin_var;
static int mem_sync_method_ping = -1;
static int mem_sync_method_pong = -1;
// mem_sync_method = 0 - no memory sync
// mem_sync_method = 1 - mb()
// mem_sync_method = 2 - msync( MS_SYNC )
// mem_sync_method = 3 - msync( MS_INVALIDATE )
// mem_sync_method = 4 - msync( MS_ASYNC )
//  = 5  (lock; addl $0,0(%%esp))  ... trying it out

#define MEM_SYNC_METHOD_0 \
	asm("")
#define MEM_SYNC_METHOD_1 \
	mb()
#define MEM_SYNC_METHOD_2 \
	msync(local_spin_var, 2, MS_SYNC)
#define MEM_SYNC_METHOD_3 \
	msync(local_spin_var, 3, MS_SYNC)
#define MEM_SYNC_METHOD_4 \
	msync(local_spin_var, 4, MS_SYNC)
#define MEM_SYNC_METHOD_5 \
	mb2()

#define MEM_SYNC_METHOD_NAME_0 "no memory sync"
#define MEM_SYNC_METHOD_NAME_1 "mb()"
#define MEM_SYNC_METHOD_NAME_2 "msync( MS_SYNC )"
#define MEM_SYNC_METHOD_NAME_3 "msync( MS_INVALIDATE )"
#define MEM_SYNC_METHOD_NAME_4 "msync( MS_ASYNC )"
#define MEM_SYNC_METHOD_NAME_5 "less expensive"

#define do_mem_sync_method(val) \
	MEM_SYNC_METHOD_##val

static const char *sync_method_string[] = {
	MEM_SYNC_METHOD_NAME_0,
	MEM_SYNC_METHOD_NAME_1,
	MEM_SYNC_METHOD_NAME_2,
	MEM_SYNC_METHOD_NAME_3,
	MEM_SYNC_METHOD_NAME_4,
	MEM_SYNC_METHOD_NAME_5
};



int make_spin_pair(int fd[2]) {
	static int spin_num = 0;

	if (spin_num == 0) {
		spin_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*spin_var = 0;
		if (mem_sync_method_ping == -1) {
			printf("No memory sync method specified.  Defaulting to '%s'\n", sync_method_string[0]);
			mem_sync_method_ping = 0;
			mem_sync_method_pong = 0;
		}
	}

	fd[0] = spin_num;
	fd[1] = spin_num;

	spin_num ++;
	return 0;
}



#define PING_LOOP_METHOD(val) \
	PING_LOOP_LABEL_ ## val: \
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
		}

#define PONG_LOOP_METHOD(val) \
	PONG_LOOP_LABEL_ ## val: \
		printf("Ponging with %s\n", MEM_SYNC_METHOD_NAME_ ## val); \
		while (1) { \
			while (*spin_var != 1) { \
			} \
			do { \
				*spin_var = 0; \
				do_mem_sync_method(val); \
			} while (0); \
		}

inline void __PINGPONG_FN do_ping_spin(int thread_num) {
	void *local_spin_var;
	static void *sync_mem_method_table[] = {
		&&PING_LOOP_LABEL_0, &&PING_LOOP_LABEL_1,
		&&PING_LOOP_LABEL_2, &&PING_LOOP_LABEL_3,
		&&PING_LOOP_LABEL_4, &&PING_LOOP_LABEL_5 };
	(void)thread_num;

	local_spin_var = spin_var;

	goto *sync_mem_method_table[mem_sync_method];

	PING_LOOP_METHOD(0);
	PING_LOOP_METHOD(1);
	PING_LOOP_METHOD(2);
	PING_LOOP_METHOD(3);
	PING_LOOP_METHOD(4);
	PING_LOOP_METHOD(5);

}

inline void __PINGPONG_FN do_pong_spin(int thread_num) {
	void *local_spin_var;
	static void *sync_mem_method_table[] = {
		&&PONG_LOOP_LABEL_0, &&PONG_LOOP_LABEL_1,
		&&PONG_LOOP_LABEL_2, &&PONG_LOOP_LABEL_3,
		&&PONG_LOOP_LABEL_4, &&PONG_LOOP_LABEL_5 };
	(void)thread_num;

	local_spin_var = spin_var;

	goto *sync_mem_method_table[mem_sync_method];

	PONG_LOOP_METHOD(0);
	PONG_LOOP_METHOD(1);
	PONG_LOOP_METHOD(2);
	PONG_LOOP_METHOD(3);
	PONG_LOOP_METHOD(4);
	PONG_LOOP_METHOD(5);
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
