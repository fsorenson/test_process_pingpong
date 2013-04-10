#ifndef __SETUP_H__
#define __SETUP_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"

int parse_thread_mode(char *arg);
int parse_comm_mode(char *arg);
int parse_opts(int argc, char *argv[]);

int setup_defaults(char *argv0);


int no_setup();
void make_pairs();

int do_send(int fd);
int do_recv(int fd);

int no_cleanup();


void set_affinity(int cpu);
int rename_thread(char *thread_name);


int do_setup();

#endif /* __SETUP_H__ */
