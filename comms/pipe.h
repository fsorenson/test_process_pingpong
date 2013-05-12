#ifndef __PIPE_H__
#define __MPIPE_H__Q_H__

#include "test_process_pingpong.h"

int comm_pre_pipe(int thread_num);
void sig_handler_pipe(int sig);
int comm_ping_pipe(int thread_num);
int comm_pong_pipe(int thread_num);
int comm_cleanup_pipe(void);

#endif
