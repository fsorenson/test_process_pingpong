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


#include "namedpipe.h"
#include "comms.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const char *namedpipe_names[] = { "/dev/shm/ping_namedpipe", "/dev/shm/pong_namedpipe" };

static int fds[2];

int comm_makepair_namedpipe(int fd[2]) {
	static int namedpipe_num = 0;
	int ret;

	if (namedpipe_num == 0) {
		if ((ret = mkfifo(namedpipe_names[0], 0666)) < 0) {
			printf("Unable to create a fifo: %s\n", strerror(errno));
			exit(1);
		}
		if ((ret = mkfifo(namedpipe_names[1], 0666)) < 0) {
			printf("Unable to create a fifo: %s\n", strerror(errno));
			exit(1);
		}

		if ((fds[0] = open(namedpipe_names[0], O_RDWR)) < 0) {
			printf("Error opening fifo %s: %s\n", namedpipe_names[0], strerror(errno));
		}
		fcntl(fds[0], F_SETFL, O_DIRECT);
		if ((fds[1] = open(namedpipe_names[1], O_RDWR)) < 0) {
			printf("Error opening fifo %s: %s\n", namedpipe_names[1], strerror(errno));
		}
		fcntl(fds[1], F_SETFL, O_DIRECT);
	}
	fd[0] = fd[1] = 0;

	namedpipe_num ++;

	return 0;
}

inline void __PINGPONG_FN comm_ping_namedpipe(int thread_num) {
	char dummy;
	(void)thread_num;

	dummy = 'X';
	while (1) {
		run_data->ping_count ++;

		while (write(fds[0], &dummy, 1) != 1) ;
		while (read(fds[1], &dummy, 1) != 1) ;
	}
}

inline void __PINGPONG_FN comm_pong_namedpipe(int thread_num) {
	char dummy;
	(void)thread_num;

	while (1) {

		while (read(fds[0], &dummy, 1) != 1) ;
		while (write(fds[1], "X", 1) != 1) ;
	}
}

int __attribute__((noreturn)) comm_cleanup_namedpipe(void) {
	close(fds[0]);
	close(fds[1]);
	unlink(namedpipe_names[0]);
	unlink(namedpipe_names[1]);
	exit(0);
}

static struct comm_mode_ops_struct comm_ops_namedpipe = {
	.comm_make_pair		= comm_makepair_namedpipe,
	.comm_ping		= comm_ping_namedpipe,
	.comm_pong		= comm_pong_namedpipe,
	.comm_cleanup		= comm_cleanup_namedpipe
};

ADD_COMM_MODE(namedpipe, "ping/pong over named pipes", &comm_ops_namedpipe);
