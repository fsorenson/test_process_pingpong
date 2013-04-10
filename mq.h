#ifndef __MQ_H__
#define __MQ_H__

#include "test_process_pingpong.h"

int make_mq_pair(int fd[2]);

inline int do_send_mq(int fd);
inline int do_recv_mq(int fd);
int cleanup_mq();

#endif
