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


#ifndef __EVENTFD_H__
#define __EVENTFD_H__

#include "comms.h"

#include <sys/syscall.h>


#ifdef SYS_eventfd
  #ifndef HAVE_EVENTFD
    #define HAVE_EVENTFD
  #endif
#endif


#ifdef HAVE_EVENTFD

#include <sys/eventfd.h>

int make_eventfd_pair(int fd[2]);

void do_ping_eventfd(int thread_num);
void do_pong_eventfd(int thread_num);

int do_send_eventfd(int fd);
int do_recv_eventfd(int fd);

#endif /* HAVE_EVENTFD */


#endif /* __EVENTFD_H__ */
