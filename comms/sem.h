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


#ifndef __SEM_H__
#define __SEM_H__

#include "comms.h"

#include <semaphore.h>

#include "test_process_pingpong.h"


int make_sem_pair(int fd[2]);
int do_begin_sem(void);

void do_ping_sem(int thread_num);
void do_pong_sem(int thread_num);
void do_ping_busysem(int thread_num);
void do_pong_busysem(int thread_num);

int do_send_sem(int fd);
int do_recv_sem(int fd);
int do_recv_busy_sem(int fd);

int cleanup_sem(void);

#endif
