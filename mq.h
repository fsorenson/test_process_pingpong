#ifndef __MQ_H__
#define __MQ_H__

#include "test_process_pingpong.h"

#include <mqueue.h>

mqd_t mqueue[2];
char mq_msg[2];
unsigned mq_prio[2];

int make_mq_pair(int fd[2]);

/*
inline int do_send_mq(int fd) {
	return ! mq_send(mqueue[fd], &mq_msg[fd], 1, mq_prio[fd]);
}

inline int do_recv_mq(int fd) {
	return (mq_receive(mqueue[fd], &mq_msg[fd], 1, &mq_prio[fd]));
}
*/
int do_send_mq(int fd);
int do_recv_mq(int fd);

int cleanup_mq();

#endif
