#include "eventfd.h"
#include "test_process_pingpong.h"

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

int __PINGPONG_FN do_ping_eventfd(int thread_num) {
	int mouth;
	int ear;
	eventfd_t efd_value;

	mouth = config.mouth[thread_num];
	ear = config.ear[thread_num];

	efd_value = 1;

	while (1) {
		run_data->ping_count ++;

		while (eventfd_write(mouth, 1) != 0);
		while (eventfd_read(ear, &efd_value) != 0);
	}
}

int __PINGPONG_FN do_pong_eventfd(int thread_num) {
	int mouth;
	int ear;
	eventfd_t efd_value;

	mouth = config.mouth[thread_num];
	ear = config.ear[thread_num];

	efd_value = 1;

	while (1) {
		while (eventfd_read(ear, &efd_value) != 0);
		while (eventfd_write(mouth, 1) != 0);
	}
}


inline int do_send_eventfd(int fd) {
        return ! eventfd_write(fd, 1);
}
inline int do_recv_eventfd(int fd) {
        eventfd_t dummy;

        return ! eventfd_read(fd, &dummy);
}


void __attribute__((constructor)) comm_add_eventfd() {
	struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("eventfd");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_eventfd_pair;
	ops.comm_do_ping = do_ping_eventfd;
	ops.comm_do_pong = do_pong_eventfd;

	comm_mode_do_initialization(&init_info, &ops);
	free(init_info.name);
}

ADD_COMM_MODE(eventfd, comm_add_eventfd);


#endif /* HAVE_EVENTFD */
