#ifndef __SPIN_H__
#define __SPIN_H__

#include "test_process_pingpong.h"

int make_spin_pair(int fd[2]);
extern inline int do_send_spin(int fd);
extern inline int do_recv_spin(int fd);
int cleanup_spin();

#endif
