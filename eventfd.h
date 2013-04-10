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

/*
inline int do_send_eventfd(int fd) {
	return ! eventfd_write(fd, 1);
}

inline int do_recv_eventfd(int fd) {
	eventfd_t dummy;

	return ! eventfd_read(fd, &dummy);
}
*/
int do_send_eventfd(int fd);
int do_recv_eventfd(int fd);

void comm_add_eventfd();

#endif /* HAVE_EVENTFD */


#endif /* __EVENTFD_H__ */
