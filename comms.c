#include "comms.h"
#include "test_process_pingpong.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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

void __attribute__((destructor)) cleanup_comm_mode_info() {
	int i;

	for (i = 0 ; i < comm_mode_count ; i ++) {
		free(comm_mode_info[i].name);
		free(comm_mode_info[i].comm_mode_init_function);
	}
	free(comm_mode_info);
	comm_mode_count = 0;
}

void comm_mode_add(char *comm_name, char *add_function_name) {
	void *ret;
//	unsigned long new_size;
	unsigned long offset;
	unsigned long zero_address;
	unsigned long zero_size;

	if ((comm_mode_count % COMM_MODE_LIST_INCREMENT) == 0) {

//		new_size = (long unsigned int)(comm_mode_count + COMM_MODE_LIST_INCREMENT) *
//		                        sizeof(struct comm_mode_info_struct);
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
	comm_mode_info[comm_mode_count].comm_mode_init_function = strdup(add_function_name);
	comm_mode_info[comm_mode_count].comm_mode_index = comm_mode_count;
	comm_mode_info[comm_mode_count].initialized = false;

//	printf("adding '%s' with initialization function '%s'\n", comm_name, add_function_name);
	comm_mode_count ++;
}
void comm_mode_add1(char *add_function_name) {
	comm_mode_add("goober", add_function_name);
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
		if (!strncmp(comm_mode_info[i].name, init_info->name, strlen(comm_mode_info[i].name))) {
//			memcpy(&comm_mode_info[i] + offsetof(struct comm_mode_info_struct, op_placeholder),
/*
			memcpy(&comm_mode_info[i].op_placeholder,
				ops,
				sizeof(struct comm_mode_ops_struct));
*/

			comm_mode_info[i].comm_init = ops->comm_init ? : comm_no_init;
			comm_mode_info[i].comm_pre = ops->comm_pre != NULL ? ops->comm_pre : comm_no_pre;
			comm_mode_info[i].comm_begin = ops->comm_begin != NULL ? ops->comm_begin : comm_no_begin;
			comm_mode_info[i].comm_make_pair = ops->comm_make_pair;
			comm_mode_info[i].comm_do_ping = ops->comm_do_ping ? : comm_do_ping_generic;
			comm_mode_info[i].comm_do_pong = ops->comm_do_pong ? : comm_do_pong_generic;
			comm_mode_info[i].comm_do_send = ops->comm_do_send ? : comm_do_send_generic;
			comm_mode_info[i].comm_do_recv = ops->comm_do_recv ? : comm_do_recv_generic;
			comm_mode_info[i].comm_interrupt = ops->comm_interrupt ? : comm_no_interrupt;
			comm_mode_info[i].comm_cleanup = ops->comm_cleanup ? : comm_no_cleanup;


			comm_mode_info[i].initialized = true;
			return;
		}
	}

	printf("ERROR: unable to initialize unknown comm mode '%s'\n", init_info->name);
}



/* this can be used to double-check that all the initializaation functions were called */
bool comm_mode_verify_all() {
	int i;
	bool result = true;

	for (i = 0 ; i < comm_mode_count ; i ++) {
		if (comm_mode_info[i].initialized == false) {
			result = false;
			printf("Initialization function '%s' for comm mode '%s' was not called!!\n",
			comm_mode_info[i].comm_mode_init_function,
			comm_mode_info[i].name);
/*		} else {
			printf("Initialization function '%s' for comm mode '%s' was called!!\n",
			comm_mode_info[i].comm_mode_init_function,
			comm_mode_info[i].name);
*/		}
	}
	return result;
}


char *get_comm_mode_name(int comm_mode_index) {
	return comm_mode_info[comm_mode_index].name;
}

int comm_no_init() {
	return 0;
}

int comm_no_pre() {
	return 0;
}

int comm_no_begin() {
	return 0;
}

inline int __PINGPONG_FN comm_do_ping_generic(int thread_num) {
	while (1) {
		run_data->ping_count ++;

		while (config.comm_do_send(config.mouth[thread_num]) != 1);
		while (config.comm_do_recv(config.ear[thread_num]) != 1);
	}
}
inline int __PINGPONG_FN comm_do_pong_generic(int thread_num) {
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

int comm_no_interrupt() {
	return 0;
}

int comm_no_cleanup() {
	return 0;
}
