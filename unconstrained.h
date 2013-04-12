#ifndef __UNCONSTRAINED_H__
#define __UNCONSTRAINED_H__

#include "comms.h"


//extern int sigsuspend (__const sigset_t *__set) __nonnull ((1));

//#include <signal.h>
#include <unistd.h>
//sigset_t unconstrained_sig_mask;

int make_unconstrained_pair(int fd[2]);

/*
* one thread just does nothing...
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/
int do_ping_unconstrained(int thread_num);
int do_pong_unconstrained(int thread_num);

int do_send_unconstrained(int fd);
int do_recv_unconstrained(int fd);
int cleanup_unconstrained();

void comm_add_unconstrained();

#endif
