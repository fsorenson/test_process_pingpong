#ifndef __FUTEX_H__
#define __FUTEX_H__

#include "comms.h"


int make_futex_test_pair(int fd[2]);

int do_ping_futex_test(int fd);
int do_pong_futex_test(int fd);
int do_send_futex_test(int fd);
int do_recv_futex_test(int fd);

int cleanup_futex_test(void);

#endif
