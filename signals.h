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


#ifndef __SIGNALS_H__
#define __SIGNALS_H__

#include "test_process_pingpong.h"

void print_backtrace(int signum);
void print_backtrace_die(int signum);
void print_backtrace2(int signum);

void setup_stop_signal(void);
void setup_child_signals(void);
void setup_crash_handler(void);
int send_sig(int pid, int sig);


#endif
