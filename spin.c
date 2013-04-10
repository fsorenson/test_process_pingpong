#include "spin.h"

#include <sys/mman.h>
#include <unistd.h>
#include <sched.h>

int *spin[2];
static unsigned int volatile *spin_var;

int make_spin_pair(int fd[2]) {
	static int spin_num = 0;

	if (spin_num == 0) {
		spin_var = mmap(NULL, sizeof(unsigned int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*spin_var = 0;
	}

//	fd[0] = spin_num;
//	fd[1] = spin_num ^ 0x01;
	fd[0] = spin_num;
	fd[1] = spin_num;

	spin_num ++;
	return 0;
}

inline int do_send_spin(int fd) {
	*spin_var = fd;

	return 1;
}
inline int do_recv_spin(int fd) {
	/* checking to see if one implementation is faster than the other...
	 * nothing to see here */

if (1) {
	return (*spin_var == fd) ? 1 : sched_yield();
} else {
	while (*spin_var != fd) {
		sched_yield();
	}
}


	return 1;
}

int cleanup_spin() {

	return 0;
}
