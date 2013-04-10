#ifndef __PIPE_H__
#define __MPIPE_H__Q_H__

#include "test_process_pingpong.h"

int pre_comm_pipe();
void sig_handler_pipe();
int comm_interrupt_pipe();
int comm_cleanup_pipe();

#endif
