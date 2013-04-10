#ifndef __SETUP_H__
#define __SETUP_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"

int parse_thread_mode(char *arg);
int parse_opts(int argc, char *argv[]);

int setup_defaults(char *argv0);


int do_comm_setup();

#endif /* __SETUP_H__ */
