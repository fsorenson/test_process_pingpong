#include "namedpipe.h"
#include "comms.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const char *namedpipe_names[] = { "/tmp/ping_namedpipe", "/tmp/pong_namedpipe" };

int comm_makepair_namedpipe(int fd[2]) {
	static int namedpipe_num = 0;
	int ret;

	if (namedpipe_num == 0) {
		if ((ret = mkfifo(namedpipe_names[0], 0666)) < 0) {
			printf("Unable to create a fifo: %s\n", strerror(errno));
			exit(1);
		}
		if ((ret = mkfifo(namedpipe_names[1], 0666)) < 0) {
			printf("Unable to create a fifo: %s\n", strerror(errno));
			exit(1);
		}
	}
	if ((fd[0] = open(namedpipe_names[namedpipe_num], O_RDWR)) < 0) {
		printf("Error opening fifo %s: %s\n", namedpipe_names[0], strerror(errno));
	}
	fcntl(fd[0], F_SETFL, O_DIRECT);
	if ((fd[1] = open(namedpipe_names[namedpipe_num ^ 1], O_RDWR)) < 0) {
		printf("Error opening fifo %s: %s\n", namedpipe_names[1], strerror(errno));
	}
	fcntl(fd[1], F_SETFL, O_DIRECT);

	namedpipe_num ++;

	return 0;
}

inline int __PINGPONG_FN comm_ping_namedpipe(int thread_num) {
	char dummy;
	(void)thread_num;

	dummy = 'X';
	while (1) {
		run_data->ping_count ++;

		while (write(config.mouth[0], &dummy, 1) != 1) ;
		while (read(config.ear[0], &dummy, 1) != 1) ;
	}
}

inline int __PINGPONG_FN comm_pong_namedpipe(int thread_num) {
	char dummy;
	(void)thread_num;

	while (1) {

		while (read(config.ear[1], &dummy, 1) != 1) ;
		while (write(config.mouth[1], "X", 1) != 1) ;
	}
}

int __attribute__((noreturn)) comm_cleanup_namedpipe(void) {
	unlink(namedpipe_names[0]);
	unlink(namedpipe_names[1]);
	exit(0);
}

static struct comm_mode_ops_struct comm_ops_namedpipe = {
	.comm_make_pair = comm_makepair_namedpipe,
	.comm_do_ping = comm_ping_namedpipe,
	.comm_do_pong = comm_pong_namedpipe,
	.comm_cleanup = comm_cleanup_namedpipe
};

ADD_COMM_MODE(namedpipe, "ping/pong over named pipes", &comm_ops_namedpipe);
