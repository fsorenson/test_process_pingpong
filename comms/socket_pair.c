#include "socket_pair.h"
#include "comms.h"

#include <string.h>
#include <sys/socket.h>

int make_socket_pair(int fd[2]) {
        socketpair(AF_UNIX, SOCK_STREAM, AF_LOCAL, fd);
        return 0;
}

void __attribute__((constructor)) comm_add_socket_pair() {
	struct comm_mode_ops_struct ops;

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_socket_pair;

	comm_mode_do_initialization("socketpair", &ops);

}

ADD_COMM_MODE(socketpair, comm_add_socket_pair);

