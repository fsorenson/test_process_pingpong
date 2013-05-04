#ifndef __COMMS_H__
#define __COMMS_H__

#include "common.h"
#include <sys/mman.h>
#include <stdlib.h>

/* don't necessarily want to limit this... just need a starting point */
#define COMM_MODE_LIST_INCREMENT	5

#define COMM_MODE_OPS				\
	int (*comm_init)(void);			\
	int (*comm_make_pair)(int fd[2]);	\
	int (*comm_pre)(int s);			\
	int (*comm_begin)(void);		\
	int (*comm_do_ping)(int s);		\
	int (*comm_do_pong)(int s);		\
	int (*comm_do_send)(int s);		\
	int (*comm_do_recv)(int s);		\
	int (*comm_interrupt)(void);		\
	int (*comm_cleanup)(void)

#define COMM_MODE_INIT_INFO \
	char *name;				\
	char *help_text;			\
	char *source_file

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

void comm_mode_add(const char *comm_name);

#define ADD_COMM_MODE(comm_name,_help_text,ops) \
	static char comm_mode_init_name_##comm_name[] = #comm_name; \
	static char comm_mode_init_help_text_##comm_name[] = _help_text; \
	static char comm_mode_init_source_file_##comm_name[] = __FILE__; \
	static struct comm_mode_init_info_struct comm_mode_info_ ##comm_name = { \
		.name = comm_mode_init_name_##comm_name, \
		.help_text = comm_mode_init_help_text_##comm_name, \
		.source_file = comm_mode_init_source_file_##comm_name \
	}; \
	void cons_comm_mode_ ## comm_name(void) __attribute__((constructor)); \
	void cons_comm_mode_ ## comm_name(void) { \
		comm_mode_add(#comm_name); \
		comm_mode_do_initialization(&comm_mode_info_ ##comm_name, ops); \
	} \
	static __attribute__((unused)) char comm_mode_init_dummy_##comm_name



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
