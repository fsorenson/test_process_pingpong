#ifndef __NOP_H__
#define __NOP_H__

#include "test_process_pingpong.h"


//extern int sigsuspend (__const sigset_t *__set) __nonnull ((1));

//#include <signal.h>
#include <unistd.h>
volatile int volatile *nop_var;
sigset_t nop_sig_mask;

int make_nop_pair(int fd[2]);

/*
* one thread just does nothing...
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/
int do_send_nop(int fd);

int do_recv_nop(int fd);

#endif
