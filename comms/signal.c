#include "signal.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include "signals.h"

#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

char comm_name_signal[] = "signal";
char comm_help_text_signal[] = "each thread sleeps until receiving a wakeup signal from the other";

static int sigs[2];
static int signal_received[2];
struct timespec sig_ts[2];

static void signal_catcher(int sig) {

	if (sigs[0] == sig) {
		signal_received[0] = 1;
		mb();
	} else if (sigs[1] == sig) {
		signal_received[1] = 1;
		mb();
	} else
		printf("WHOA!!! got signal %d\n", sig);
}

int make_signal_pair(int fd[2]) {
	static int sig_num = 0;

	if (sig_num == 0) {
		sigs[0] = SIGRTMAX-1;
		sigs[1] = SIGRTMAX-2;
	}

	fd[0] = fd[1] = sig_num;

	signal_received[sig_num] = 0;

	sig_num ++;
	return 0;
}

int do_pre_signal(int thread_num) {
	struct sigaction sa;
	int sig;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sig = sigs[thread_num];
	sa.sa_handler = &signal_catcher;

	sigaction(sig, &sa, NULL);

//printf("thread %d listening for sig %d\n", thread_num, sig);
	return 0;
}

inline int __PINGPONG_FN do_ping_signal(int thread_num) {
        sigset_t signal_mask;
	(void) thread_num;

	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGUSR1);
	sigdelset(&signal_mask, sigs[0]);

	while (1) {
		run_data->ping_count ++;

		send_sig(run_data->thread_info[thread_num ^ 1].pid, sigs[thread_num ^ 1]);


		while (! signal_received[thread_num]) {
//sigsuspend(&signal_mask);
		}
		signal_received[thread_num] = 0;
	}
}

inline int __PINGPONG_FN do_pong_signal(int thread_num) {
	(void) thread_num;

	while (1) {
		while (! signal_received[thread_num]) {
		}
		signal_received[thread_num] = 0;

		send_sig(run_data->thread_info[thread_num ^ 1].pid, sigs[thread_num ^ 1]);
	}
}

int __CONST cleanup_signal(void) {

	return 0;
}

static struct comm_mode_init_info_struct comm_info_signal = {
	.name = comm_name_signal,
	.help_text = comm_help_text_signal
};

static struct comm_mode_ops_struct comm_ops_signal = {
	.comm_make_pair = make_signal_pair,
	.comm_pre = do_pre_signal,
	.comm_do_ping = do_ping_signal,
	.comm_do_pong = do_pong_signal,
	.comm_cleanup = cleanup_signal
};

void comm_add_signal(void) {
	comm_mode_do_initialization(&comm_info_signal, &comm_ops_signal);
}

NEW_ADD_COMM_MODE(signal, "each thread sleeps until receiving a wakeup signal from the other", &comm_ops_signal);
