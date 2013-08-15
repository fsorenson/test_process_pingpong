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

#define DEBUG_SPIN 0

#if DEBUG_SPIN
#define DEBUG_BUFFER_LEN 200
static char debug_buffer[DEBUG_BUFFER_LEN];
#define dprintf(_f, _a...) printf(_f, ## _a)

#else

#define dprintf(_f, _a...) ((void)0)

#endif

int volatile *spin_var;
static int mem_sync_method_ping = -1;
static int mem_sync_method_pong = -1;

#define MEM_SYNC_METHOD_NAME_0 "no memory sync"
#define MEM_SYNC_METHOD_NAME_1 "mb()"
#define MEM_SYNC_METHOD_NAME_2 "msync( MS_SYNC )"
#define MEM_SYNC_METHOD_NAME_3 "msync( MS_INVALIDATE )"
#define MEM_SYNC_METHOD_NAME_4 "msync( MS_ASYNC )"
#define MEM_SYNC_METHOD_NAME_5 "__sync_synchronize"
#define MEM_SYNC_METHOD_NAME_6 "asm memory clobber"
//  = 5  (lock; addl $0,0(%%esp))  ... trying it out

#define MEM_SYNC_METHOD_0 \
	asm("")
#define MEM_SYNC_METHOD_1 \
	mb()
#define MEM_SYNC_METHOD_2 \
	msync(spin_var, sizeof(int), MS_SYNC)
#define MEM_SYNC_METHOD_3 \
	msync(spin_var, sizeof(int), MS_INVALIDATE)
#define MEM_SYNC_METHOD_4 \
	msync(spin_var, sizeof(int), MS_ASYNC)
#define MEM_SYNC_METHOD_5 \
	__sync_synchronize()
#define MEM_SYNC_METHOD_6 \
	mb2()

#define do_mem_sync_method(val) \
	MEM_SYNC_METHOD_##val

typedef void (*spin_pingpong_method_t)(void);

#pragma pack(4)
struct spin_memsync_struct {
	const int sequence; /* ordering of the functions */
	const char *fn_name; /* function name */
	const char *description;
	spin_pingpong_method_t ping_fn;
	spin_pingpong_method_t pong_fn;
};
#pragma pack()

#define SPIN_SECTION				spin

#define SECTION_START(section)			__PASTE(__start_,section)
#define SECTION_STOP(section)			__PASTE(__stop_,section)

#define SPIN_FUNCTION_NAME_SECTION		__PASTE(SPIN_SECTION,_syms)				/* the section where the names of the symbols are stored */
#define SPIN_FUNCTION_NAME_SECTION_STRING	__STR(SPIN_FUNCTION_NAME_SECTION)			/* the section where the names of the symbols are stored */
#define SPIN_FUNCTION_NAME_SECTION_START	SECTION_START(SPIN_FUNCTION_NAME_SECTION)
#define SPIN_FUNCTION_NAME_SECTION_STOP		SECTION_STOP(SPIN_FUNCTION_NAME_SECTION)

#define SPIN_FUNCTION_DESCRIPTION_SECTION	__PASTE(SPIN_SECTION,_descr)				/* the section where function description strings are stored */
#define SPIN_FUNCTION_DESCRIPTION_SECTION_STRING	__STR(SPIN_FUNCTION_DESCRIPTION_SECTION)		/* the section where function description strings are stored */
#define SPIN_FUNCTION_DESCRIPTION_SECTION_START		SECTION_START(SPIN_FUNCTION_DESCRIPTION_SECTION)
#define SPIN_FUNCTION_DESCRIPTION_SECTION_STOP		SECTION_STOP(SPIN_FUNCTION_DESCRIPTION_SECTION)

#define SPIN_FUNCTION_CODE_SECTION		__PASTE(SPIN_SECTION,_text)				/* the section where the executable is stored */
#define SPIN_FUNCTION_CODE_SECTION_STRING	__STR(SPIN_FUNCTION_CODE_SECTION)
#define SPIN_FUNCTION_CODE_SECTION_START	SECTION_START(SPIN_FUNCTION_CODE_SECTION)
#define SPIN_FUNCTION_CODE_SECTION_STOP		SECTION_STOP(SPIN_FUNCTION_CODE_SECTION)

#define SPIN_FUNCTION_TABLE_SECTION		__PASTE(SPIN_SECTION,_ftab)				/* section where function table is stored */
#define SPIN_FUNCTION_TABLE_SECTION_STRING	__STR(SPIN_FUNCTION_TABLE_SECTION)
#define SPIN_FUNCTION_TABLE_SECTION_START	SECTION_START(SPIN_FUNCTION_TABLE_SECTION)
#define SPIN_FUNCTION_TABLE_SECTION_STOP	SECTION_STOP(SPIN_FUNCTION_TABLE_SECTION)

#define SPIN_MAKE_NAME(_sym,_b) __PASTE(__PASTE(_spin_, _sym), _b)
#define SPIN_MAKE_NAME_STRING(_sym,_b) __STR(SPIN_MAKE_NAME(_sym,_b))

#define SPIN_MAKE_PING_NAME(_sym)		SPIN_MAKE_NAME(_sym, _ping_fn)
#define SPIN_MAKE_PONG_NAME(_sym)		SPIN_MAKE_NAME(_sym, _pong_fn)

#define SPIN_FUNCTION_ATTRIBUTES		__USED __attribute__((section(__STR(SPIN_FUNCTION_CODE_SECTION_STRING)) ))
#define SPIN_FUNCTION_SYMBOL_NAME_ATTRIBUTES	__USED __attribute__((section(SPIN_FUNCTION_NAME_SECTION_STRING), aligned(1) ))
#define SPIN_FUNCTION_DESCRIPTION_ATTRIBUTES	__USED __attribute__((section(SPIN_FUNCTION_DESCRIPTION_SECTION_STRING), aligned(1) ))
#define SPIN_FUNCTION_TABLE_ATTRIBUTES		__USED __attribute__((section(SPIN_FUNCTION_TABLE_SECTION_STRING), used, aligned(4)))

#define SPIN_FUNCTION_DESCRIPTION(_sym,_desc)						\
	static const char SPIN_MAKE_NAME(_sym, _descr)[]				\
		SPIN_FUNCTION_DESCRIPTION_ATTRIBUTES \
		= _desc

#define SPIN_FUNCTION_SYMBOL_NAME(_sym) \
	static const char SPIN_MAKE_NAME(_sym, _name)[]					\
		SPIN_FUNCTION_SYMBOL_NAME_ATTRIBUTES \
		= SPIN_MAKE_NAME_STRING(_sym, _name)

#define SPIN_PING_FUNCTION_NAME_STRING(_sym) \
	static const char SPIN_MAKE_PING_NAME(_sym)[]					\
		SPIN_FUNCTION_SYMBOL_NAME_ATTRIBUTES \
		= SPIN_MAKE_PING_NAME(_sym)

#define SPIN_PONG_FUNCTION_NAME_STRING(_sym) \
	static const char SPIN_MAKE_PONG_NAME(_sym)[]					\
		SPIN_FUNCTION_SYMBOL_NAME_ATTRIBUTES \
		= SPIN_MAKE_PONG_NAME(_sym)


#define SPIN_PING_FUNCTION_DECL(_sym) \
	spin_pingpong_method_t __USED __NORETURN SPIN_MAKE_PING_NAME(_sym)(void)

#define SPIN_PING_FUNCTION_CODE(_sym,_sync_code)	\
	while (1) {					\
		run_data->ping_count ++;		\
\
		do {					\
			*spin_var = 1;			\
			_sync_code;			\
		} while (0);				\
		while (*spin_var != 0) {		\
		}					\
	}

#define SPIN_PONG_FUNCTION_DECL(_sym) \
	spin_pingpong_method_t __USED __NORETURN SPIN_MAKE_PONG_NAME(_sym)(void)

#define SPIN_PONG_FUNCTION_CODE(_sym,_sync_code)	\
	while (1) { \
		while (*spin_var != 1) { \
		} \
		do { \
			*spin_var = 0; \
			_sync_code; \
		} while (0); \
	}

#define SPIN_FUNCTION_TABLE_ENTRY(_sequence, _sym) \
	static const struct spin_memsync_struct SPIN_MAKE_NAME(_sym, _ftab_e)			\
			SPIN_FUNCTION_TABLE_ATTRIBUTES					\
		= {									\
			_sequence,							\
			__STR(_sym),							\
			SPIN_MAKE_NAME(_sym, _descr),					\
			(spin_pingpong_method_t)SPIN_MAKE_PING_NAME(_sym),					\
			(spin_pingpong_method_t)SPIN_MAKE_PONG_NAME(_sym)		\
		}

#define spin_define_function(_sequence,_sym,_desc,_code)					\
	SPIN_PING_FUNCTION_DECL(_sym);								\
	SPIN_PING_FUNCTION_DECL(_sym) {								\
		SPIN_PING_FUNCTION_CODE(_sym, _code);						\
	};											\
	SPIN_PONG_FUNCTION_DECL(_sym);								\
	SPIN_PONG_FUNCTION_DECL(_sym) {								\
		SPIN_PONG_FUNCTION_CODE(_sym, _code);						\
	};											\
\
	SPIN_FUNCTION_DESCRIPTION(_sym,_desc);							\
	SPIN_FUNCTION_SYMBOL_NAME(_sym);							\
	SPIN_FUNCTION_TABLE_ENTRY(_sequence, _sym);

static struct spin_memsync_struct **spin_memsync_info;
static int spin_memsync_method_count = -1;

extern struct spin_memsync_struct SPIN_FUNCTION_TABLE_SECTION_START;
extern struct spin_memsync_struct SPIN_FUNCTION_TABLE_SECTION_STOP;

extern void *SPIN_FUNCTION_NAME_SECTION_START;
extern void *SPIN_FUNCTION_NAME_SECTION_STOP;

extern void *SPIN_FUNCTION_CODE_SECTION_START;
extern void *SPIN_FUNCTION_CODE_SECTION_STOP;

extern void *SPIN_FUNCTION_DESCRIPTION_SECTION_START;
extern void *SPIN_FUNCTION_DESCRIPTION_SECTION_STOP;


static struct spin_memsync_struct *mem_sync_method_ping_info = 0;
static struct spin_memsync_struct *mem_sync_method_pong_info = 0;

static int compare_memsync_structs(const void *p1, const void *p2) {
	struct spin_memsync_struct *s1 = * (struct spin_memsync_struct * const *)p1;
	struct spin_memsync_struct *s2 = * (struct spin_memsync_struct * const *)p2;
	int ret;

	if (s1->sequence < s2->sequence)
		return -1;
	if (s1->sequence > s2->sequence)
		return 1;
	ret = strcmp(s1->fn_name, s2->fn_name);
	if (ret != 0)
		return ret;
	ret = strcmp(s1->description, s2->description);
	if (ret != 0)
		return ret;
	return (p1 < p2);
}

static int setup_memsync_info(void) {
	int i;
	char *p;

	if (spin_memsync_method_count != -1)
		return spin_memsync_method_count;

	spin_memsync_method_count =
		(((void *)&SPIN_FUNCTION_TABLE_SECTION_STOP) - ((void *)&SPIN_FUNCTION_TABLE_SECTION_START))
		/ sizeof(struct spin_memsync_struct);

#if DEBUG_SPIN
	printf("Found %d memsync methods\n", spin_memsync_method_count);
	printf("size of spin_memsync_struct = %lu\n", sizeof(struct spin_memsync_struct));
	printf("allocated %d x %lu bytes\n", spin_memsync_method_count, sizeof(struct spin_memsync_struct *));
#endif

	spin_memsync_info = calloc(spin_memsync_method_count, sizeof(struct spin_memsync_struct *));

	p = (void *)&SPIN_FUNCTION_TABLE_SECTION_START;
	for (i = 0 ; i < spin_memsync_method_count ; i ++) {
		spin_memsync_info[i] = (struct spin_memsync_struct *) (p + sizeof(struct spin_memsync_struct) * i);
	}

	qsort(spin_memsync_info, spin_memsync_method_count, sizeof(struct spin_memsync_struct *), &compare_memsync_structs);
#if DEBUG_SPIN
	for (i = 0 ; i < spin_memsync_method_count ; i ++) {
		printf("#%d - %s (sequence %d)\n", i, spin_memsync_info[i]->description, spin_memsync_info[i]->sequence);

	}
#endif
	return spin_memsync_method_count;
}

static void drop_memsync_info(void) {
	if (spin_memsync_method_count != -1)
		free(spin_memsync_info);
	spin_memsync_method_count = -1;
}

static const char *sync_method_string[] = {
	MEM_SYNC_METHOD_NAME_0,
	MEM_SYNC_METHOD_NAME_1,
	MEM_SYNC_METHOD_NAME_2,
	MEM_SYNC_METHOD_NAME_3,
	MEM_SYNC_METHOD_NAME_4,
	MEM_SYNC_METHOD_NAME_5,
	MEM_SYNC_METHOD_NAME_6
};

static int sync_method_count = sizeof(sync_method_string) / sizeof(sync_method_string[0]);

int comm_spin_show_options(const char *indent_string) {
	int i;

	printf("%s options which may be specified for both ping/pong (use '##') or each (use '##,##'):\n", indent_string);
	for (i = 0 ; i < sync_method_count ; i ++) {
		if (! (i % 2) )
			printf("%s", indent_string);
		printf(" %d - %-20s", i, sync_method_string[i]);
		if (i % 2)
			printf("\n");
	}
	if (sync_method_count % 2)
		printf("\n");
	return 0;
}

int comm_spin_parse_options(const char *option_string) {
	int max_value;
	long value;
	char *p_remainder;

	setup_memsync_info();

	max_value = spin_memsync_method_count - 1;

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
			p_remainder, spin_memsync_info[mem_sync_method_ping]->description);
	}

	if (mem_sync_method_ping == -1) {
		printf("No memory sync method specified.  Defaulting to '%s'\n", sync_method_string[0]);
		mem_sync_method_ping = 0;
		mem_sync_method_pong = 0;
	}
	mem_sync_method_ping_info = spin_memsync_info[mem_sync_method_ping];
	mem_sync_method_pong_info = spin_memsync_info[mem_sync_method_pong];

	drop_memsync_info();

	return 0;
}

int make_spin_pair(int fd[2]) {
	static int spin_num = 0;

	if (spin_num == 0) {
		spin_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*spin_var = 0;
		if (mem_sync_method_ping == -1) {
			setup_memsync_info();

			printf("No memory sync method specified.  Defaulting to '%s'\n", sync_method_string[0]);
			mem_sync_method_ping = 0;
			mem_sync_method_pong = 0;

			mem_sync_method_ping_info = spin_memsync_info[mem_sync_method_ping];
			mem_sync_method_pong_info = spin_memsync_info[mem_sync_method_pong];

			drop_memsync_info();
		}
	}

	fd[0] = spin_num;
	fd[1] = spin_num;

	spin_num ++;
	return 0;
}



#define PING_LOOP_METHOD(val) \
	PING_LOOP_LABEL_ ## val: \
		dprintf("Pinging with %s\n", MEM_SYNC_METHOD_NAME_ ## val); \
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
		dprintf("Ponging with %s\n", MEM_SYNC_METHOD_NAME_ ## val); \
		while (1) { \
			while (*spin_var != 1) { \
			} \
			do { \
				*spin_var = 0; \
				do_mem_sync_method(val); \
			} while (0); \
		}

void __PINGPONG_FN comm_ping_spin(int thread_num) {
	static void *sync_mem_method_table[] = {
		&&PING_LOOP_LABEL_0, &&PING_LOOP_LABEL_1,
		&&PING_LOOP_LABEL_2, &&PING_LOOP_LABEL_3,
		&&PING_LOOP_LABEL_4, &&PING_LOOP_LABEL_5,
		&&PING_LOOP_LABEL_6
		};
	(void)thread_num;


	goto *sync_mem_method_table[mem_sync_method_ping];

	PING_LOOP_METHOD(0);
	PING_LOOP_METHOD(1);
	PING_LOOP_METHOD(2);
	PING_LOOP_METHOD(3);
	PING_LOOP_METHOD(4);
	PING_LOOP_METHOD(5);
	PING_LOOP_METHOD(6);

}

void __PINGPONG_FN comm_pong_spin(int thread_num) {
	static void *sync_mem_method_table[] = {
		&&PONG_LOOP_LABEL_0, &&PONG_LOOP_LABEL_1,
		&&PONG_LOOP_LABEL_2, &&PONG_LOOP_LABEL_3,
		&&PONG_LOOP_LABEL_4, &&PONG_LOOP_LABEL_5,
		&&PONG_LOOP_LABEL_6
		};
	(void)thread_num;


	goto *sync_mem_method_table[mem_sync_method_pong];

	PONG_LOOP_METHOD(0);
	PONG_LOOP_METHOD(1);
	PONG_LOOP_METHOD(2);
	PONG_LOOP_METHOD(3);
	PONG_LOOP_METHOD(4);
	PONG_LOOP_METHOD(5);
	PONG_LOOP_METHOD(6);
}

int __CONST cleanup_spin(void) {
	munmap((void *)spin_var, sizeof(int));
	return 0;
}

static struct comm_mode_ops_struct comm_ops_spin = {
	.comm_make_pair		= make_spin_pair,
	.comm_ping		= comm_ping_spin,
	.comm_pong		= comm_pong_spin,
	.comm_show_options	= comm_spin_show_options,
	.comm_parse_options	= comm_spin_parse_options,
	.comm_cleanup		= cleanup_spin
};

ADD_COMM_MODE(spin, "busy-wait on a shared variable", &comm_ops_spin);
