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


#ifndef __PIPE_H__
#define __PIPE_H__

#include "test_process_pingpong.h"

int comm_pre_pipe(int thread_num);
void sig_handler_pipe(int sig);
void comm_ping_pipe(int thread_num);
void comm_pong_pipe(int thread_num);
int comm_cleanup_pipe(void);

#endif
