#ifndef __SPIN_H__
#define __SPIN_H__

#include "comms.h"

int make_spin_unrolled_pair(int fd[2]);

int do_ping_spin_unrolled(int thread_num);
int do_pong_spin_unrolled(int thread_num);

int cleanup_spin_unrolled(void);

#endif
