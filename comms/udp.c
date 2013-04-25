#include "udp.h"
#include "comms.h"

#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>

int make_udp_pair(int fd[2]) {
        int fds, fdc;
        socklen_t addr_len;

        union {
                struct sockaddr_in inaddr;
                struct sockaddr addr;
        } a;
        addr_len = sizeof(a.inaddr);

        fds = socket(PF_INET, SOCK_DGRAM, 0);

        memset(&a, 0, sizeof(a));
        a.inaddr.sin_family = AF_INET;
        a.inaddr.sin_port = 0;
        a.inaddr.sin_addr.s_addr = INADDR_ANY;

        bind(fds, &a.addr, addr_len);

        fdc = socket(PF_INET, SOCK_DGRAM, 0);

        memset(&a.inaddr, 0, sizeof(a));
        a.inaddr.sin_family = AF_INET;
        a.inaddr.sin_port = 0;
        a.inaddr.sin_addr.s_addr = INADDR_ANY;
        bind(fds, &a.inaddr, addr_len);

//      addr_len = sizeof(saddr);
        getsockname(fds, &a.addr, &addr_len);
        connect(fdc, &a.addr, addr_len);

        addr_len = sizeof(a.inaddr);
        getsockname(fdc, &a.addr, &addr_len);
        connect(fds, &a.addr, addr_len);

        fd[0] = fds;
        fd[1] = fdc;
        return 0;
}

void __attribute__((constructor)) comm_add_udp() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("udp");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_udp_pair;

	comm_mode_do_initialization(&init_info, &ops);
	free(init_info.name);
}

ADD_COMM_MODE(udp, comm_add_udp);
