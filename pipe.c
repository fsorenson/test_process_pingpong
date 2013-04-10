#include "pipe.h"

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#ifndef __GNU_SOURCE
#define __GNU_SOURCE
#endif

int pre_comm_pipe() {
	struct sigaction sa;

write(1, "setup sigpipe\n", 14);
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sig_handler_pipe;
	sigaction(SIGPIPE, &sa, NULL);

	return 0;
}

void sig_handler_pipe() {
write(1, "sigpipe\n", 8);
	comm_cleanup_pipe();
}

int comm_interrupt_pipe() {
	close(config.mouth[0]);
	close(config.mouth[1]);
write(1, "sent interrupts\n", 16);

	return 0;
}

int comm_cleanup_pipe() {
write(1, "bye\n", 4);
	exit(0);
}
