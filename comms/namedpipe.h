#ifndef __NAMEDPIPE_H__

#include "test_process_pingpong.h"

int comm_makepair_namedpipe(int fd[2]);
int comm_ping_namedpipe(int thread_num);
int comm_pong_namedpipe(int thread_num);

int comm_cleanup_namedpipe(void);

#endif
