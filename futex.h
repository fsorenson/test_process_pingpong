#ifndef __FUTEX_H__
#define __FUTEX_H__

#include "test_process_pingpong.h"

#include <linux/futex.h>

int *futex_id[2];
int futex_vals[2];

int make_futex_pair(int fd[2]);

int do_send_futex(int fd);
int do_recv_futex(int fd);

int cleanup_futex();

#endif
