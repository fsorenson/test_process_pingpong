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


#ifndef __NOP_H__
#define __NOP_H__

#include "comms.h"

/*
* one thread just does nothing...
* the other also does nothing, but sleeps too...
* lazy, good-for-nothing threads
*/

int comm_nop_show_options(const char *indent_string);
int comm_nop_parse_options(const char *option_string);

int make_nop_pair(int fd[2]);

void do_ping_nop(int thread_num);
void do_pong_nop(int thread_num);

int cleanup_nop(void);

#endif
