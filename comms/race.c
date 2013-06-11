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


#include "race.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <sys/mman.h>
#include <string.h>

int volatile *race_var;

int make_race_pair(int fd[2]) {
	static int race_num = 0;

	if (race_num == 0) {
		race_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*race_var = 0;
	}

	fd[0] = race_num;
	fd[1] = race_num;

	race_num ++;
	return 0;
}


inline int __PINGPONG_FN do_ping_race1(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		*race_var = 1;
	}
}

inline int __PINGPONG_FN do_ping_race2(int thread_num) {
	(void)thread_num;

	while (1) {
		run_data->ping_count ++;

		*race_var = 1;
	}
}

inline int __PINGPONG_FN do_pong_race1(int thread_num) {
	(void)thread_num;
	while (1) {
		*race_var = 0;
	}
}

inline int __PINGPONG_FN do_pong_race2(int thread_num) {
	(void)thread_num;
	while (1) {
		run_data->ping_count ++;

		*race_var = 0;
	}
}

int __CONST cleanup_race(void) {

	return 0;
}

static struct comm_mode_ops_struct comm_ops_race1 = {
	.comm_make_pair = make_race_pair,
	.comm_do_ping = do_ping_race1,
	.comm_do_pong = do_pong_race1,
	.comm_cleanup = cleanup_race
};
static struct comm_mode_ops_struct comm_ops_race2 = {
	.comm_make_pair = make_race_pair,
	.comm_do_ping = do_ping_race2,
	.comm_do_pong = do_pong_race2,
	.comm_cleanup = cleanup_race
};

ADD_COMM_MODE(race1, "both threads repeatedly write their own value--no pingpong", &comm_ops_race1);
ADD_COMM_MODE(race2, "both threads repeatedly write their own value _AND_ increment the counter--no pingpong", &comm_ops_race2);
