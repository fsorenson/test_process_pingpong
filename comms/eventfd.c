/*
	This file is part of test_process_pingpong

	Copyright 2013 Frank Sorenson (frank@tuxrocks.com)


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


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

inline void __PINGPONG_FN comm_ping_eventfd(int thread_num) {
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

inline void __PINGPONG_FN comm_pong_eventfd(int thread_num) {
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

static struct comm_mode_ops_struct comm_ops_eventfd = {
	.comm_make_pair		= make_eventfd_pair,
	.comm_ping		= comm_ping_eventfd,
	.comm_pong		= comm_pong_eventfd
};

ADD_COMM_MODE(eventfd, "", &comm_ops_eventfd);


#endif /* HAVE_EVENTFD */
