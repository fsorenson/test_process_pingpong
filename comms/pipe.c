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


#include "pipe.h"
#include "comms.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

int comm_pre_pipe(int thread_num) {
	struct sigaction sa;
	(void)thread_num;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sig_handler_pipe;
	sigaction(SIGPIPE, &sa, NULL);

	return 0;
}

void __attribute__((noreturn)) sig_handler_pipe(int sig) {
	(void)sig;

	comm_cleanup_pipe();
}

inline int __PINGPONG_FN comm_ping_pipe(int thread_num) {
	char dummy;
	(void)thread_num;

	dummy = 'X';
	while (1) {
		run_data->ping_count ++;

		while (write(config.mouth[0], &dummy, 1) != 1);
		while (read(config.ear[0], &dummy, 1) != 1);
	}
}

inline int __PINGPONG_FN comm_pong_pipe(int thread_num) {
	char dummy;
	(void)thread_num;

	dummy = 'X';
	while (1) {
		while (read(config.ear[1], &dummy, 1) != 1);
		while (write(config.mouth[1], &dummy, 1) != 1);
	}
}


int __attribute__((noreturn)) comm_cleanup_pipe(void) {
	exit(0);
}

static struct comm_mode_ops_struct comm_ops_pipe = {
	.comm_make_pair = pipe,
	.comm_pre = comm_pre_pipe,
	.comm_do_ping = comm_ping_pipe,
	.comm_do_pong = comm_pong_pipe,
	.comm_cleanup = comm_cleanup_pipe
};

ADD_COMM_MODE(pipe, "pipe() ping/pong", &comm_ops_pipe);
