#include "eventfd.h"

#include <unistd.h>
#include <sys/syscall.h>


#ifdef SYS_eventfd
  #define HAVE_EVENTFD
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


#endif /* HAVE_EVENTFD */
