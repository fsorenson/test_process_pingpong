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

#define MEM_SYNC_METHOD_NAME_0 "no memory sync"
#define MEM_SYNC_METHOD_NAME_1 "mb()"
#define MEM_SYNC_METHOD_NAME_2 "msync( MS_SYNC )"
#define MEM_SYNC_METHOD_NAME_3 "msync( MS_INVALIDATE )"
#define MEM_SYNC_METHOD_NAME_4 "msync( MS_ASYNC )"
#define MEM_SYNC_METHOD_NAME_5 "less expensive"
//  = 5  (lock; addl $0,0(%%esp))  ... trying it out

static const char *sync_method_string[] = {
	MEM_SYNC_METHOD_NAME_0,
	MEM_SYNC_METHOD_NAME_1,
	MEM_SYNC_METHOD_NAME_2,
	MEM_SYNC_METHOD_NAME_3,
	MEM_SYNC_METHOD_NAME_4,
	MEM_SYNC_METHOD_NAME_5
};

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

#define do_mem_sync_method(val) \
	MEM_SYNC_METHOD_##val

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
	volatile void *local_spin_var;
	static void *sync_mem_method_table[] = {
		&&PING_LOOP_LABEL_0, &&PING_LOOP_LABEL_1,
		&&PING_LOOP_LABEL_2, &&PING_LOOP_LABEL_3,
		&&PING_LOOP_LABEL_4, &&PING_LOOP_LABEL_5 };
	(void)thread_num;

	local_spin_var = spin_var;

	goto *sync_mem_method_table[mem_sync_method_ping];

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

	goto *sync_mem_method_table[mem_sync_method_pong];

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

int comm_spin_show_options(const char *indent_string) {
	int sync_method_count = sizeof(sync_method_string) / sizeof(sync_method_string[0]);
	int i;

	printf("%s options which may be specified for both ping/pong (use '##') or each (use '##,##'):\n", indent_string);
	for (i = 0 ; i < sync_method_count ; i ++) {
		printf("%s %d - %s\n", indent_string, i, sync_method_string[i]);
	}
	return 0;
}

int comm_spin_parse_options(const char *option_string) {
	int max_value = (sizeof(sync_method_string) / sizeof(sync_method_string[0])) - 1;
	long value;
	char *p_remainder;

	value = strtol(option_string, &p_remainder, 10);
	if (((value < 0) || (value > max_value)) || (option_string == p_remainder)) {
		printf("Unable to correctly parse '%s'...  expected an integer from 0-%d\n",
			option_string, max_value);
		exit(1);
	}
	mem_sync_method_ping = value;

	if (*p_remainder == '\0') {
		mem_sync_method_pong = value;
	} else if (*p_remainder == ',') {
		p_remainder ++;
		value = strtol(p_remainder, NULL, 10);
		if ((value < 0) || (value > max_value)) {
			printf("Option value %ld is outside the appropriate range (0-%d)\n", value, max_value);
			exit(1);
		}
		mem_sync_method_pong = value;
	} else {
		printf("Unrecognized option value specified ('%s').  Pong falling back to use '%s' as with ping\n",
			p_remainder, sync_method_string[mem_sync_method_ping]);
	}
	return 0;
}


static struct comm_mode_ops_struct comm_ops_spin = {
	.comm_make_pair		= make_spin_pair,
	.comm_do_ping		= do_ping_spin,
	.comm_do_pong		= do_pong_spin,
	.comm_show_options	= comm_spin_show_options,
	.comm_parse_options	= comm_spin_parse_options,
	.comm_cleanup		= cleanup_spin
};

ADD_COMM_MODE(spin, "busy-wait on a shared variable", &comm_ops_spin);
