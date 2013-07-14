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


#include "nop.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include <stdio.h>

volatile int *nop_var;

static int nop_variety = -1;

struct timespec nop_ts;

static const char *nop_variety_string[] = {
	"set, then test variable",
	"only set variable",
	"only test variable",
	"only increments counter",
	"loop-unrolled increment"
};

static int nop_variety_count = sizeof(nop_variety_string) / sizeof(nop_variety_string[0]);

int comm_nop_show_options(const char *indent_string) {
	int i;

	printf("%sThe following options specify the laziness of the ping process (pong only sleeps)\n", indent_string);
	for (i = 0 ; i < nop_variety_count ; i ++) {
		if (! (i % 2) )
			printf("%s", indent_string);
		printf(" %d - %-25s", i, nop_variety_string[i]);
		if (i % 2)
			printf("\n");
	}
	if (nop_variety_count % 2)
		printf("\n");
	return 0;
}

int comm_nop_parse_options(const char *option_string) {
	int max_value = nop_variety_count - 1;
	long value;
	char *p_remainder;

	value = strtol(option_string, &p_remainder, 10);
	if ((value < 0) || (value > max_value) || (option_string == p_remainder)) {
		printf("Unable to parse '%s' -- expected an integer from 0-%d\n",
			option_string, max_value);
		exit(1);
	}

	nop_variety = value;

	return 0;
}

int make_nop_pair(int fd[2]) {
	static int nop_num = 0;

	if (nop_num == 0) {
		nop_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*nop_var = 0;

		nop_ts.tv_sec = 0;
		nop_ts.tv_nsec = 1000000;

		if (nop_variety == -1) {
			printf("No 'nop' ping laziness specified.  Defaulting to '%s'\n", nop_variety_string[0]);
			nop_variety = 0;
		}
	}

	fd[0] = nop_num;
	fd[1] = nop_num;

	nop_num ++;
	return 0;
}


/*
* one thread just does nothing...  in various ways
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/
//inline void __attribute__((optimize("O0")))  do_ping_nop(int thread_num) { // this can be used during debugging
inline void __PINGPONG_FN do_ping_nop(int thread_num) {
	(void) thread_num;

	*nop_var = 1;

	switch (nop_variety) {
		case 0:
			while (1) {
				run_data->ping_count ++;

				*nop_var = *nop_var;
				__asm__ __volatile__("");
				do { } while (*nop_var != *nop_var);
				__asm__ __volatile__("");
			}
		case 1:
			while (1) {
				run_data->ping_count ++;

				do {} while (unlikely(*nop_var != 1));
				__asm__ __volatile__("");
			}
		case 2:
			while (1) {
				run_data->ping_count ++;

				*nop_var = 1;
				__asm__ __volatile__("");
			}
		case 3:
			while (1) {
				run_data->ping_count ++;
			}
		case 4: {
			run_data->ping_count ++;
			asm("addq $1, %rdx");
			asm("movq %rdx, (%rax)");
			asm("addq $0x1,%rdx");		// increment
			asm("movq %rdx,(%rax)");	// store
			asm("addq $0x1,%rdx");		// increment
			asm("movq %rdx,(%rax)");	// store
			asm("addq $0x1,%rdx");		// increment
			asm("movq %rdx,(%rax)");	// store

//	asm("mov run_data->ping_count,%rax");
//	asm("mov (%rax),%rdx");
			while (1) {

				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
				asm("addq $0x1,%rdx");		// increment
				asm("movq %rdx,(%rax)");	// store
			}
		}
		default:
			exit(1);
	}
}

inline void __PINGPONG_FN do_pong_nop(int thread_num) {
	(void) thread_num;

	while (1) {
		nanosleep(&nop_ts, NULL);
	}
}
int __CONST cleanup_nop(void) {

	return 0;
}

static struct comm_mode_ops_struct comm_ops_nop = {
	.comm_show_options	= comm_nop_show_options,
	.comm_parse_options	= comm_nop_parse_options,
	.comm_make_pair		= make_nop_pair,
	.comm_do_ping = do_ping_nop,
	.comm_do_pong = do_pong_nop,
	.comm_cleanup = cleanup_nop
};

ADD_COMM_MODE(nop, "no ping/pong - first thread does various levels of nothing, second thread sleeps", &comm_ops_nop);
