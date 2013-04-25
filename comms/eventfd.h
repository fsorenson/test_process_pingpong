#ifndef __EVENTFD_H__
#define __EVENTFD_H__

#include "comms.h"

#include <sys/syscall.h>


#ifdef SYS_eventfd
  #ifndef HAVE_EVENTFD
    #define HAVE_EVENTFD
  #endif
#endif


#ifdef HAVE_EVENTFD

#include <sys/eventfd.h>

int make_eventfd_pair(int fd[2]);

int do_ping_eventfd(int thread_num);
int do_pong_eventfd(int thread_num);

int do_send_eventfd(int fd);
int do_recv_eventfd(int fd);

void comm_add_eventfd(void);

#endif /* HAVE_EVENTFD */


#endif /* __EVENTFD_H__ */
