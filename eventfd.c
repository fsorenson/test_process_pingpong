#include "eventfd.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

#ifdef SYS_eventfd
  #ifndef HAVE_EVENTFD
    #define HAVE_EVENTFD
  #endif
#endif


#ifdef HAVE_EVENTFD

#include <sys/eventfd.h>

int make_eventfd_pair(int fd[2]) {
        fd[0] = eventfd(0, 0);
        fd[1] = dup(fd[0]);
        return 0;
}


inline int do_send_eventfd(int fd) {
        return ! eventfd_write(fd, 1);
}
inline int do_recv_eventfd(int fd) {
        eventfd_t dummy;

        return ! eventfd_read(fd, &dummy);
}


void __attribute__((constructor)) comm_add_eventfd() {
	struct comm_mode_ops_struct ops;

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_eventfd_pair;

	comm_mode_do_initialization("eventfd", &ops);

}

ADD_COMM_MODE(eventfd, comm_add_eventfd);


#endif /* HAVE_EVENTFD */
