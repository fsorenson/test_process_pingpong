#ifndef __TCP_H__
#define __TCP_H__

#include "comms.h"


int new_make_tcp_pair(int fd[2]);

int make_tcp_pair(int fd[2]);

int do_ping_tcp(int thread_num);
int do_pong_tcp(int thread_num);

#endif
