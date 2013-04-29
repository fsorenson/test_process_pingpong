#ifndef __NOP_H__
#define __NOP_H__

#include "comms.h"

/*
* one thread just does nothing...
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/

int make_nop_pair(int fd[2]);

int do_ping_nop1(int thread_num);
int do_ping_nop2(int thread_num);
int do_ping_nop3(int thread_num);
int do_ping_nop4(int thread_num);
int do_pong_nop(int thread_num);

int cleanup_nop(void);

void comm_add_nop1(void);
void comm_add_nop2(void);
void comm_add_nop3(void);
void comm_add_nop4(void);

#endif
