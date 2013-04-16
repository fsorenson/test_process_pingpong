#ifndef __RACE_H__
#define __RACE_H__

#include "comms.h"

int make_race_pair(int fd[2]);

int do_ping_race(int thread_num);
int do_pong_race(int thread_num);

int cleanup_race();

void comm_add_race();

#endif
