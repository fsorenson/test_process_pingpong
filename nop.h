#ifndef __NOP_H__
#define __NOP_H__

#include "test_process_pingpong.h"

int make_nop_pair(int fd[2]);
inline int do_send_nop(int fd);
inline int do_recv_nop(int fd);

#endif
