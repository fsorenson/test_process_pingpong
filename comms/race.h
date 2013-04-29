#ifndef __RACE_H__
#define __RACE_H__

#include "comms.h"

int make_race_pair(int fd[2]);

int do_ping_race1(int thread_num);
int do_ping_race2(int thread_num);
int do_pong_race1(int thread_num);
int do_pong_race2(int thread_num);

int cleanup_race(void);

void comm_add_race1(void);
void comm_add_race2(void);

#endif
