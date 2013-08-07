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


#include "comms.h"
#include "test_process_pingpong.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

struct comm_mode_info_struct *comm_mode_info = NULL;
int comm_mode_count = 0;

int parse_comm_mode(char *arg) {
	int i;

	config.comm_mode_index = 0;
	for (i = 0 ; i < comm_mode_count ; i ++)
		if (! strcmp(arg, comm_mode_info[i].name)) {
			config.comm_mode_index = i;
			return i;
		}
	printf("Unknown communication mode '%s'.  Falling back to default\n", arg);

	return -1;
}

void __attribute__((destructor)) cleanup_comm_mode_info(void) {
	int i;

	if (comm_mode_info != 0) {
		for (i = 0 ; i < comm_mode_count ; i ++) {
			if (comm_mode_info[i].help_text != 0)
				free(comm_mode_info[i].help_text);
			comm_mode_info[i].help_text = 0;
			if (comm_mode_info[i].name != 0)
				free(comm_mode_info[i].name);
			comm_mode_info[i].name = 0;
		}
		free(comm_mode_info);
	}
	comm_mode_info = 0;
	comm_mode_count = 0;
}

void comm_mode_add(const char *comm_name) {
	void *ret;
	unsigned long offset;
	unsigned long zero_address;
	unsigned long zero_size;

	if ((comm_mode_count % COMM_MODE_LIST_INCREMENT) == 0) {

		ret = realloc(comm_mode_info,
			(long unsigned int)(comm_mode_count + COMM_MODE_LIST_INCREMENT) *
			sizeof(struct comm_mode_info_struct));
		if (ret == NULL) {
			printf("Unable to reallocate memory\n");
			exit(-1);

		}
		if (errno) {
			printf("errno is nonzero, for some reason\n");
		}
		comm_mode_info = (struct comm_mode_info_struct *)ret;

		offset = sizeof(struct comm_mode_info_struct) * ((long unsigned int)comm_mode_count);
		zero_address = (unsigned long)(char *)comm_mode_info + offset;
		zero_size = sizeof(struct comm_mode_info_struct) * COMM_MODE_LIST_INCREMENT;

		ret = memset((void *)zero_address, 0, zero_size);
		if (ret != (void *)zero_address) {
			printf("somethin' fishy...  expected 0x%08lx to equal 0x%08lx\n",
				(unsigned long)ret, zero_address);
		}
	}

	comm_mode_info[comm_mode_count].name = strdup(comm_name);
	comm_mode_info[comm_mode_count].comm_mode_index = comm_mode_count;
	comm_mode_info[comm_mode_count].initialized = false;

	comm_mode_count ++;
}

/* call this from within the initializer to note that the initialization function has been called */
void comm_mode_mark_initialized(char *comm_mode_name) {
	int i;

	for (i = 0 ; i < comm_mode_count ; i ++) {
		if (!strncmp(comm_mode_info[i].name, comm_mode_name, strlen(comm_mode_info[i].name))) {
			comm_mode_info[i].initialized = true;
			return;
		}
	}

	printf("ERROR: unable to initialize unknown comm mode '%s'\n", comm_mode_name);
}

void comm_mode_do_initialization(struct comm_mode_init_info_struct *init_info, struct comm_mode_ops_struct *ops) {
	int i;

	for (i = 0 ; i < comm_mode_count ; i ++) {
		if (strlen(comm_mode_info[i].name) != strlen(init_info->name))
			continue;
		if (!strncmp(comm_mode_info[i].name, init_info->name, strlen(comm_mode_info[i].name))) {
			if (init_info->help_text != 0)
				comm_mode_info[i].help_text = strdup(init_info->help_text);

			comm_mode_info[i].comm_init = ops->comm_init ? ops->comm_init : comm_no_init;
			comm_mode_info[i].comm_pre = ops->comm_pre != NULL ? ops->comm_pre : comm_no_pre;
			comm_mode_info[i].comm_begin = ops->comm_begin != NULL ? ops->comm_begin : comm_no_begin;
			comm_mode_info[i].comm_make_pair = ops->comm_make_pair;
			comm_mode_info[i].comm_ping = ops->comm_ping ? ops->comm_ping : comm_ping_generic;
			comm_mode_info[i].comm_pong = ops->comm_pong ? ops->comm_pong : comm_pong_generic;
			comm_mode_info[i].comm_do_send = ops->comm_do_send ? ops->comm_do_send : comm_do_send_generic;
			comm_mode_info[i].comm_do_recv = ops->comm_do_recv ? ops->comm_do_recv : comm_do_recv_generic;
			comm_mode_info[i].comm_cleanup = ops->comm_cleanup ? ops->comm_cleanup : comm_no_cleanup;
			comm_mode_info[i].comm_show_options = ops->comm_show_options ? ops->comm_show_options : comm_show_no_options;
			comm_mode_info[i].comm_parse_options = ops->comm_parse_options ? ops->comm_parse_options : comm_parse_no_options;


			comm_mode_info[i].initialized = true;
			return;
		}
	}

	printf("ERROR: unable to initialize unknown comm mode '%s'\n", init_info->name);
}

/* this can be used to double-check that all the initializaation functions were called */
bool comm_mode_verify_all(void) {
	int i;
	bool result = true;

	for (i = 0 ; i < comm_mode_count ; i ++) {
		if (comm_mode_info[i].initialized == false) {
			result = false;
			printf("Initialization function for comm mode '%s' was not called!!\n",
			comm_mode_info[i].name);
		}
	}
	return result;
}


char * __PURE get_comm_mode_name(int comm_mode_index) {
	return comm_mode_info[comm_mode_index].name;
}

int __CONST comm_no_init(void) {
	return 0;
}

int __CONST comm_no_pre(int thread_num) {
	(void)thread_num;

	return 0;
}

int __CONST comm_no_begin(void) {
	return 0;
}

inline void __PINGPONG_FN comm_ping_generic(int thread_num) {
	while (1) {
		run_data->ping_count ++;

		while (config.comm_do_send(config.mouth[thread_num]) != 1);
		while (config.comm_do_recv(config.ear[thread_num]) != 1);
	}
}
inline void __PINGPONG_FN comm_pong_generic(int thread_num) {
	while (1) {
		while (config.comm_do_recv(config.ear[thread_num]) != 1);
		while (config.comm_do_send(config.mouth[thread_num]) != 1);
	}
}

inline int comm_do_send_generic(int fd) {
	return (int)write(fd, "X", 1);
}

inline int comm_do_recv_generic(int fd) {
	char dummy;

	return (int)read(fd, &dummy, 1);
}

int __CONST comm_no_cleanup(void) {
	return 0;
}
int __CONST comm_show_no_options(const char *indent_string) {
	(void)indent_string;
	return 0;
}
int __CONST comm_parse_no_options(const char *option_string) {
	(void)option_string;

	/* config.comm_mode_index should have been set previous to calling this */
	printf("Comm mode '%s' has no options.\n", get_comm_mode_name(config.comm_mode_index));

	return 0;
}
