#include "tcp.h"

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

int make_tcp_pair(int fd[2]) {
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

