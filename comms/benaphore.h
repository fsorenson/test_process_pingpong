#ifndef __BENAPHORE_H__
#define __BENAPHORE_H__

#include "comms.h"

#include <semaphore.h>

#include "test_process_pingpong.h"


int make_ben_pair(int fd[2]);

int do_send_ben(int fd);
int do_recv_ben(int fd);

int cleanup_ben(void);

#endif
