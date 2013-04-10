#ifndef __EVENTFD_H__
#define __EVENTFD_H__

#include "test_process_pingpong.h"

int make_eventfd_pair(int fd[2]);

extern inline int do_send_eventfd(int fd);
extern inline int do_recv_eventfd(int fd);

#endif
