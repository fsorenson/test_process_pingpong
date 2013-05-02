#include "socket_pair.h"
#include "comms.h"

#include <string.h>
#include <sys/socket.h>


int make_socket_pair(int fd[2]) {
        socketpair(AF_UNIX, SOCK_STREAM, AF_LOCAL, fd);
        return 0;
}

static struct comm_mode_ops_struct comm_ops_socketpair = {
	.comm_make_pair = make_socket_pair
};

ADD_COMM_MODE(socketpair, "", &comm_ops_socketpair);
