#ifndef __SPIN_H__
#define __SPIN_H__

#include "test_process_pingpong.h"

int make_spin_pair(int fd[2]);
inline int do_send_spin(int fd);
inline int do_recv_spin(int fd);
int cleanup_spin();

#endif
