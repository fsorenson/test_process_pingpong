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


/*
        test_process_pingpong: program to test 'ping-pong' performance
		between two processes on a host

        by Frank Sorenson (frank@tuxrocks.com) 2013
*/

#include "test_process_pingpong.h"
#include "setup.h"
#include "threads_startup.h"

#include "signals.h"

#include "units.h"
#include "sched.h"

#include "comms.h"

#include <stdio.h>
#include <stdlib.h>




int main(int argc, char *argv[]) {
	int ret;

//	printf("looks like we've got %d comm modes\n", comm_mode_count);

	ret = comm_mode_verify_all();
	if (ret == false)
		exit_fail("failed to initialize all modules\n");

	setup_crash_handler();

	setup_defaults(argv[0]);

	parse_opts(argc, argv);

	do_comm_setup();

//	init_mlockall();

	start_threads();

	return 0;
}
