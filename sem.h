#ifndef __SEM_H__
#define __SEM_H__

#include "comms.h"

#include <semaphore.h>

#include "test_process_pingpong.h"


int make_sem_pair(int fd[2]);

int do_send_sem(int fd);
int do_recv_sem(int fd);
int do_recv_busy_sem(int fd);

int cleanup_sem();

void comm_add_sem();
void comm_add_busy_sem();

#endif
