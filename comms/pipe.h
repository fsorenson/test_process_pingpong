#ifndef __PIPE_H__
#define __MPIPE_H__Q_H__

#include "test_process_pingpong.h"

int comm_pre_pipe(int thread_num);
void sig_handler_pipe(int sig);
int comm_interrupt_pipe(void);
int comm_cleanup_pipe(void);

void comm_add_pipe(void);

#endif
