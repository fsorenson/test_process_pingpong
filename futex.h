#ifndef __FUTEX_H__
#define __FUTEX_H__

#include "test_process_pingpong.h"


int make_futex_pair(int fd[2]);

int do_send_futex(int fd);
int do_recv_futex(int fd);

int cleanup_futex();

#endif
