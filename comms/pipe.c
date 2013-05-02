#include "pipe.h"
#include "comms.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

char comm_name_pipe[] = "pipe";

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

void __attribute__((noreturn)) sig_handler_pipe(int sig) {
	(void)sig;

write(1, "sigpipe\n", 8);
	comm_cleanup_pipe();
}

int comm_interrupt_pipe(void) {
	close(config.mouth[0]);
	close(config.mouth[1]);
write(1, "sent interrupts\n", 16);

	return 0;
}

int __attribute__((noreturn)) comm_cleanup_pipe(void) {
write(1, "bye\n", 4);
	exit(0);
}

static struct comm_mode_init_info_struct comm_info_pipe = {
	.name = comm_name_pipe,
};

static struct comm_mode_ops_struct comm_ops_pipe = {
	.comm_make_pair = pipe,
	.comm_pre = comm_pre_pipe,
	.comm_interrupt = comm_interrupt_pipe,
	.comm_cleanup = comm_cleanup_pipe
};

void comm_add_pipe(void) {
	comm_mode_do_initialization(&comm_info_pipe, &comm_ops_pipe);
}

NEW_ADD_COMM_MODE(pipe, "", &comm_ops_pipe);
