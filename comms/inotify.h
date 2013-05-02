#ifndef __INOTIFY_H__
#define __INOTIFY_H__

#include "comms.h"

#include "test_process_pingpong.h"


int make_inotify_pair(int fd[2]);

int do_ping_inotify(int thread_num);
int do_pong_inotify(int thread_num);

int cleanup_inotify(void);

#endif
