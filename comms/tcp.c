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


#include "tcp.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

int tcp_fds[2];

int make_tcp_pair(int fd[2]) {
	static int tcp_num = 0;

	int ret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_storage remote_addr;
	socklen_t addr_size;
	int sockfd1;
	int yes_flag = 1;

//	if (config.verbosity >= 1) printf("Setting up pair %d\n", tcp_num);

	if (tcp_num == 0) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		if ((ret = getaddrinfo("127.0.0.1", "3490", &hints, &res)) != 0) {
			printf("error with getaddrinfo(): %s\n", gai_strerror(ret));
			exit(-1);
		}


		if ((sockfd1 = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
			printf("Error creating socket: %s\n", strerror(errno));
			exit(-1);
		}
		if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &yes_flag, sizeof(int)) == -1) {
			printf("Error with setsockopt: %s\n", strerror(errno));
			exit(-1);
		}
		if (bind(sockfd1, res->ai_addr, res->ai_addrlen) == -1) {
			printf("Error with bind: %s\n", strerror(errno));
			exit(-1);
		}
		if (listen(sockfd1, 1) == -1) {
			printf("Error with listen: %s\n", strerror(errno));
			exit(-1);
		}



		if ((tcp_fds[1] = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
			printf("Error creating socket: %s\n", strerror(errno));
			exit(-1);
		}
		if (connect(tcp_fds[1], res->ai_addr, res->ai_addrlen) == -1) {
			printf("Error with connect: %s\n", strerror(errno));
			exit(-1);
		}

			/* */
		addr_size = sizeof(remote_addr);
		if ((tcp_fds[0] = accept(sockfd1, (struct sockaddr *)&remote_addr, &addr_size)) == -1) {
			printf("Error with accept: %s\n", strerror(errno));
			exit(-1);
		}

		if (config.uid == 0) {
			if (setsockopt(tcp_fds[0], SOL_SOCKET, TCP_NODELAY, &yes_flag, sizeof(int)) == -1) {
				printf("Error with setsockopt(TCP_NODELAY): %s\n", strerror(errno));
				exit(-1);
			}
			if (setsockopt(tcp_fds[0], SOL_SOCKET, TCP_QUICKACK, &yes_flag, sizeof(int)) == -1) {
				printf("Error with setsockopt (TCP_QUICKACK): %s\n", strerror(errno));
				exit(-1);
			}
		}
	}

	fd[0] = tcp_fds[tcp_num];
	fd[1] = tcp_fds[tcp_num ^ 1];

	tcp_num ++;

	return 0;
}

inline int __PINGPONG_FN do_ping_tcp(int thread_num) {
	char dummy = 'X';
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		while (send(tcp_fds[0], &dummy, 1, MSG_DONTWAIT) != 1);
		while (recv(tcp_fds[0], &dummy, 1, MSG_DONTWAIT) != 1);
	}
}

inline int __PINGPONG_FN do_pong_tcp(int thread_num) {
	char dummy = 'X';
	(void)thread_num;

	while (1) {
		while (recv(tcp_fds[1], &dummy, 1, MSG_DONTWAIT) != 1);
		while (send(tcp_fds[1], &dummy, 1, 0) != 1);
	}
}

static struct comm_mode_ops_struct comm_ops_tcp = {
	.comm_make_pair = make_tcp_pair,
	.comm_do_ping = do_ping_tcp,
	.comm_do_pong = do_pong_tcp
};

ADD_COMM_MODE(tcp, "TCP ping/pong", &comm_ops_tcp);
