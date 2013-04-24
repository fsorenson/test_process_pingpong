#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include "comms.h"


int make_signal_pair(int fd[2]);

int do_pre_signal(int thread_num);
int do_ping_signal(int thread_num);
int do_pong_signal(int thread_num);

int cleanup_signal();

void comm_add_signal();

#endif
