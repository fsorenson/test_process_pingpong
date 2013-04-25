#ifndef __SEM_H__
#define __SEM_H__

#include "comms.h"

#include <semaphore.h>

#include "test_process_pingpong.h"


int make_sem_pair(int fd[2]);
int do_begin_sem(void);

int do_ping_sem(int thread_num);
int do_pong_sem(int thread_num);
int do_ping_busysem(int thread_num);
int do_pong_busysem(int thread_num);

int do_send_sem(int fd);
int do_recv_sem(int fd);
int do_recv_busy_sem(int fd);

int cleanup_sem(void);

void comm_add_sem(void);
void comm_add_busy_sem(void);

#endif
