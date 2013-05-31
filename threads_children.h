#ifndef __THREADS_CHILDREN_H__
#define __THREADS_CHILDREN_H__

#include "test_process_pingpong.h"

void do_thread_work(int thread_num);
int thread_function(void *argument);
void *pthread_function(void *argument);


#endif
