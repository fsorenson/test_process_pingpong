#include "spin.h"

#include <sys/mman.h>
#include <unistd.h>
#include <sched.h>

int *spin[2];
extern volatile int volatile *spin_var;

int make_spin_pair(int fd[2]) {
	static int spin_num = 0;

	if (spin_num == 0) {
		spin_var = mmap(NULL, sizeof(int),
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		*spin_var = 0;
	}

	fd[0] = spin_num;
	fd[1] = spin_num;

	spin_num ++;
	return 0;
}

inline int do_send_spin(int fd) {

	*spin_var = fd;
	__sync_synchronize();

	return 1;
}
inline int do_recv_spin(int fd) {

	while (*spin_var != fd)
		__sync_synchronize();

	return 1;
}

int cleanup_spin() {

	return 0;
}
