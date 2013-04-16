#ifndef __NOP_H__
#define __NOP_H__

#include "comms.h"


//extern int sigsuspend (__const sigset_t *__set) __nonnull ((1));

//#include <signal.h>
#include <unistd.h>
//sigset_t nop_sig_mask;

int make_nop_pair(int fd[2]);

/*
* one thread just does nothing...
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/
int do_ping_nop(int thread_num);
int do_pong_nop(int thread_num);

int do_send_nop(int fd);
int do_recv_nop(int fd);
int cleanup_nop();

void comm_add_nop();

#endif
