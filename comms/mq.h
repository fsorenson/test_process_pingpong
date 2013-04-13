#ifndef __MQ_H__
#define __MQ_H__

#include "comms.h"

int make_mq_pair(int fd[2]);

int do_send_mq(int fd);
int do_recv_mq(int fd);

int cleanup_mq();

void comm_add_mq();

#endif
