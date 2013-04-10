#include "socket_pair.h"

//#include <sys/types.h>
#include <sys/socket.h>

int make_socket_pair(int fd[2]) {
        socketpair(AF_UNIX, SOCK_STREAM, AF_LOCAL, fd);
        return 0;
}

