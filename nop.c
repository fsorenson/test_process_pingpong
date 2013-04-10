#include "nop.h"
#include <sys/mman.h>
#include <unistd.h>


int make_nop_pair(int fd[2]) {
	static int nop_num = 0;


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
		sleep(1);
	} else {
//		sched_yield();
	}
	return 1;
}
inline int do_recv_nop(int fd) {

	return 1;
}

int cleanup_nop() {

	return 0;
}

