#include "tcp.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>

int old_make_tcp_pair(int fd[2]) {
        int fds, fdc;
        socklen_t addr_len;

        union {
                struct sockaddr_in inaddr;
                struct sockaddr addr;
        } a;
        addr_len = sizeof(a.inaddr);

        fds = socket(PF_INET, SOCK_STREAM, 0);

        memset(&a, 0, sizeof(a));
        a.inaddr.sin_family = AF_INET;
        a.inaddr.sin_port = 0;
//      a.inaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        a.inaddr.sin_addr.s_addr = INADDR_ANY;

        bind(fds, &a.addr, addr_len);
        getsockname(fds, &a.addr, &addr_len);
        listen(fds, 1);

        fdc = socket(PF_INET, SOCK_STREAM, 0);

        connect(fdc, &a.addr, addr_len);

        fd[0] = accept(fds, 0, 0);
        fd[1] = fdc;
        close(fds);
        return 0;
}

int tcp_fds[2];

int make_tcp_pair(int fd[2]) {
	static int tcp_num = 0;

	int ret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_storage remote_addr;
	socklen_t addr_size;
	int sockfd1;
	int yes = 1;

	if (config.verbosity >= 1) printf("Setting up pair %d\n", tcp_num);

	if (tcp_num == 0) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		if ((ret = getaddrinfo("127.0.0.1", "3491", &hints, &res)) != 0) {
			printf("error with getaddrinfo(): %s\n", gai_strerror(ret));
			exit(-1);
		}


		if ((sockfd1 = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
			printf("Error creating socket: %m\n");
			exit(-1);
		}
		if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
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
	}

	fd[0] = fd[1] = tcp_fds[tcp_num];

	tcp_num ++;

	return 0;
}
