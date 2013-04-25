#ifndef __YIELD_H__
#define __YIELD_H__

#include "comms.h"

int make_yield_pair(int fd[2]);

int do_ping_yield(int thread_num);
int do_pong_yield(int thread_num);
int do_ping_yield_nop(int thread_num);
int do_pong_yield_nop(int thread_num);

//int do_send_yield(int fd);
//int do_recv_yield(int fd);


void comm_add_yield(void);
void comm_add_yield_nop(void);

#endif
