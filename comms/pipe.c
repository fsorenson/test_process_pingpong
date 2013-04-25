#include "pipe.h"
#include "comms.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>


int comm_pre_pipe(int thread_num) {
	struct sigaction sa;
	(void)thread_num;

write(1, "setup sigpipe\n", 14);
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sig_handler_pipe;
	sigaction(SIGPIPE, &sa, NULL);

	return 0;
}

void __attribute__((noreturn)) sig_handler_pipe() {
write(1, "sigpipe\n", 8);
	comm_cleanup_pipe();
}

int comm_interrupt_pipe() {
	close(config.mouth[0]);
	close(config.mouth[1]);
write(1, "sent interrupts\n", 16);

	return 0;
}

int __attribute__((noreturn)) comm_cleanup_pipe() {
write(1, "bye\n", 4);
	exit(0);
}


void __attribute__((constructor)) comm_add_pipe() {
	struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("pipe");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = pipe;
	ops.comm_pre = comm_pre_pipe;
	ops.comm_interrupt = comm_interrupt_pipe;
	ops.comm_cleanup = comm_cleanup_pipe;

	comm_mode_do_initialization(&init_info, &ops);
	free(init_info.name);
}

ADD_COMM_MODE(pipe, comm_add_pipe);
