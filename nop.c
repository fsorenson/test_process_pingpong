#include "nop.h"
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>


volatile int volatile *nop_var;

int make_nop_pair(int fd[2]) {
	static int nop_num = 0;

	if (nop_num == 0) {
		nop_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*nop_var = 0;
	}

	fd[0] = nop_num;
	fd[1] = nop_num;

	nop_num ++;
	return 0;
}

inline int do_send_nop(int fd) {
	/* one thread just does nothing...
	 * the other also does nothing, but sleeps too...
	 * lazy, good-for-nothing threads */

	if (fd == 1) {
		sigset_t signal_mask;

		sigfillset(&signal_mask);
		sigdelset(&signal_mask, SIGINT);

		while (stats->stop != true)
			sigsuspend(&signal_mask);

	} else {
		*nop_var ^= 1;
		__sync_synchronize();
	}
	return 1;
}
inline int do_recv_nop(int fd) {
	(void)fd;

	return 1;
}

int cleanup_nop() {

	return 0;
}
