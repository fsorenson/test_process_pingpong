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

        bind(fds, (struct sockaddr *)&a.addr, addr_len);

        fdc = socket(PF_INET, SOCK_DGRAM, 0);

        memset(&a.inaddr, 0, sizeof(a));
        a.inaddr.sin_family = AF_INET;
        a.inaddr.sin_port = 0;
        a.inaddr.sin_addr.s_addr = INADDR_ANY;
        bind(fds, (struct sockaddr *)&a.inaddr, addr_len);

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

static struct comm_mode_ops_struct comm_ops_udp = {
	.comm_make_pair = make_udp_pair
};

ADD_COMM_MODE(udp, "UDP ping/pong", &comm_ops_udp);
