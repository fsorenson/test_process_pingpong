#ifndef __COMMS_H__
#define __COMMS_H__

#include "common.h"
#include <sys/mman.h>
#include <stdlib.h>

/* don't necessarily want to limit this... just need a starting point */
#define COMM_MODE_LIST_INCREMENT	5

#define COMM_MODE_OPS				\
	char *comm_mode_init_function;		\
	int op_placeholder;			\
	int op_placeholder2;			\
	int (*comm_init)();			\
	int (*comm_make_pair)(int fd[2]);	\
	int (*comm_pre)(int s);			\
	int (*comm_begin)();			\
	int (*comm_do_ping)(int s);		\
	int (*comm_do_pong)(int s);		\
	int (*comm_do_send)(int s);		\
	int (*comm_do_recv)(int s);		\
	int (*comm_interrupt)();		\
	int (*comm_cleanup)()

#define COMM_MODE_INIT_INFO \
	char *name;				\
	char *help_text

struct comm_mode_ops_struct {
	COMM_MODE_OPS;
};

struct comm_mode_init_info_struct {
	COMM_MODE_INIT_INFO;
};

struct comm_mode_info_struct {
	COMM_MODE_OPS;

	COMM_MODE_INIT_INFO;

	int comm_mode_index;

	bool initialized;

	char dummy1[4];
	char dummy2[7];
	char dummy3[8];
};

extern struct comm_mode_info_struct *comm_mode_info;
extern int comm_mode_count;

void cleanup_comm_mode_info(void);
int parse_comm_mode(char *arg);
char *get_comm_mode_name(int index);

void comm_mode_add(const char *comm_name, const char *add_function_name);

#define ADD_COMM_MODE1(function_name) \
  void cons_ ## function_name() __attribute__ ((constructor)); \
  void cons_ ## function_name() { comm_mode_add2(#function_name); }


#define ADD_COMM_MODE(comm_name,add_function_name) \
	void cons_ ## add_function_name(void) __attribute__((constructor)); \
	void cons_ ## add_function_name() { comm_mode_add(#comm_name, #add_function_name); }

int comm_no_init(void);
int comm_no_pre(int thread_num);
int comm_no_begin(void);

int comm_do_ping_generic(int thread_num);
int comm_do_pong_generic(int thread_num);
int comm_do_send_generic(int fd);
int comm_do_recv_generic(int fd);

int comm_no_interrupt(void);
int comm_no_cleanup(void);

void comm_mode_do_initialization(struct comm_mode_init_info_struct *init_info, struct comm_mode_ops_struct *ops);
void comm_mode_mark_initialized(char *comm_mode_name);
bool comm_mode_verify_all(void);

#endif
