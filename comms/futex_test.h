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


#ifndef __FUTEX_H__
#define __FUTEX_H__

#include "comms.h"


int make_futex_test_pair(int fd[2]);

void comm_ping_futex_test(int fd);
void comm_pong_futex_test(int fd);
int do_send_futex_test(int fd);
int do_recv_futex_test(int fd);

int cleanup_futex_test(void);

#endif
