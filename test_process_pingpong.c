/*
        test_process_pingpong: program to test 'ping-pong' performance
		between two processes on a host

        by Frank Sorenson (frank@tuxrocks.com) 2013
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "setup.h"
#include "threads.h"
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
	if (ret == false) {
		printf("failed to initialize all modules\n");
		exit(-1);

	}

	setup_crash_handler();

	setup_defaults(argv[0]);

	parse_opts(argc, argv);

	do_comm_setup();

	start_threads();

	return 0;
}
