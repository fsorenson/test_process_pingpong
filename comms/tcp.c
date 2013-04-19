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


int tcp_fds[2];

int new_make_tcp_pair(int fd[2]) {
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
			printf("Error creating socket: %m\n");
			exit(-1);
		}
		if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &yes_flag, sizeof(int)) == -1) {
			printf("Error with setsockopt: %m\n");
			exit(-1);
		}
		if (bind(sockfd1, res->ai_addr, res->ai_addrlen) == -1) {
			printf("Error with bind: %m\n");
			exit(-1);
		}
		if (listen(sockfd1, 1) == -1) {
			printf("Error with listen: %m\n");
			exit(-1);
		}



		if ((tcp_fds[1] = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
			printf("Error creating socket: %m\n");
			exit(-1);
		}
		if (connect(tcp_fds[1], res->ai_addr, res->ai_addrlen) == -1) {
			printf("Error with connect: %m\n");
			exit(-1);
		}

			/* */
		addr_size = sizeof(remote_addr);
		if ((tcp_fds[0] = accept(sockfd1, (struct sockaddr *)&remote_addr, &addr_size)) == -1) {
			printf("Error with accept: %m\n");
			exit(-1);
		}

		if (config.uid == 0) {
			if (setsockopt(tcp_fds[0], SOL_SOCKET, TCP_NODELAY, &yes_flag, sizeof(int)) == -1) {
				printf("Error with setsockopt(TCP_NODELAY): %m\n");
				exit(-1);
			}
			if (setsockopt(tcp_fds[0], SOL_SOCKET, TCP_QUICKACK, &yes_flag, sizeof(int)) == -1) {
				printf("Error with setsockopt (TCP_QUICKACK): %m\n");
				exit(-1);
			}
		}
	}

	fd[0] = tcp_fds[tcp_num];
	fd[1] = tcp_fds[tcp_num ^ 1];

	tcp_num ++;

	return 0;
}

int make_tcp_pair(int fd[2]) {
	return new_make_tcp_pair(fd);
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


//ADD_COMM_MODE(comm_add_tcp);

void __attribute__((constructor)) comm_add_tcp() {
	struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = "tcp";

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_tcp_pair;
	ops.comm_do_ping = do_ping_tcp;
	ops.comm_do_pong = do_pong_tcp;

	comm_mode_do_initialization(&init_info, &ops);
}

ADD_COMM_MODE(tcp, comm_add_tcp);

