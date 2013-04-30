#include "socket_pair.h"
#include "comms.h"

#include <string.h>
#include <sys/socket.h>

char comm_name_socketpair[] = "socketpair";

int make_socket_pair(int fd[2]) {
        socketpair(AF_UNIX, SOCK_STREAM, AF_LOCAL, fd);
        return 0;
}

static struct comm_mode_init_info_struct comm_info_socketpair = {
	.name = comm_name_socketpair
};

static struct comm_mode_ops_struct comm_ops_socketpair = {
	.comm_make_pair = make_socket_pair
};

void __attribute__((constructor)) comm_add_socket_pair(void) {
	comm_mode_do_initialization(&comm_info_socketpair, &comm_ops_socketpair);
}

ADD_COMM_MODE(socketpair, comm_add_socket_pair);

