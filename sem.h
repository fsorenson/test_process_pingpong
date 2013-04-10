#ifndef __SEM_H__
#define __SEM_H__

#include <semaphore.h>

#include "test_process_pingpong.h"

//extern const char *sem_names[];


int make_sem_pair(int fd[2]);

int do_send_sem(int fd);
int do_recv_sem(int fd);

int cleanup_sem();

#endif
