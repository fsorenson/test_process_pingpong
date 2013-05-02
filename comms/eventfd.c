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

char comm_name_eventfd[] = "eventfd";


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

static struct comm_mode_init_info_struct comm_info_eventfd = {
	.name = comm_name_eventfd
};

static struct comm_mode_ops_struct comm_ops_eventfd = {
	.comm_make_pair = make_eventfd_pair,
	.comm_do_ping = do_ping_eventfd,
	.comm_do_pong = do_pong_eventfd
};

void comm_add_eventfd(void) {
	comm_mode_do_initialization(&comm_info_eventfd, &comm_ops_eventfd);
}

NEW_ADD_COMM_MODE(eventfd, "", &comm_ops_eventfd);


#endif /* HAVE_EVENTFD */
