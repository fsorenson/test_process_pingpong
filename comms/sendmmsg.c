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


#include "sendmmsg.h"
#include "comms.h"

#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>

int make_sendmmsg_pair(int fd[2]) {
	int ret;
	struct addrinfo *ainfo;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM,
		.ai_protocol = IPPROTO_UDP,
		.ai_flags = AI_PASSIVE,
	};
	int fd;
	int i;
	char buf[100];

	ret = getaddrinfo("127.0.0.1", "5000", &hints, &ainfo);
	if (ret)
		exit_fail("error using getaddrinfo: %s\n", gai_strerror(ret));

	fd = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);
	if (fd == -1)
		exit_fail("creating socket: error: %s\n", strerror(errno));

	char sendbuf[1][1];
	struct iovec[1][1];
	struct mmsghdr dgrams[1];

	memset(sendbuf, 0, sizeof(sendbuf));
	memset(iovec, 0, sizeof(iovec));
	memset(dgrams, 0, sizeof(dgrams));
	sendbuf[0][0] = 'X';
	iovec[0][0].iov_base = sendbuf[0];
	iovec[0][0].iov_len = 1;
	dgrams[0].msg_hdr.msg_iov = iovec[0];
	dgrams[0].msg_hdr.msg_iovlen = 1;
	if (ainfo->ai_addr) {
		dgrams[0].msg_hdr.msg_name = ainfo->ai_addr;
		dgrams[0].msg_hdr.msg_namelen = sizeof(*ainfo->ai_addr);
	}


        fd[0] = fd;
        return 0;
}

/*
sendmmsg(fd, dgrams, 1, 0);
*/
inline int do_send_sendmmsg(int fd) {

	return ! eventfd_write(fd, 1);
}
inline int do_recv_sendmmsg(int fd) {

	return ! eventfd_read(fd, &dummy);
}


static struct comm_mode_ops_struct comm_ops_sendmmsg = {
	.comm_make_pair = make_sendmmsg_pair
};

ADD_COMM_MODE(sendmmsg, "", &comm_ops_sendmmsg);
