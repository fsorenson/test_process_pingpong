#ifndef __SPIN_H__
#define __SPIN_H__

#include "test_process_pingpong.h"

volatile int volatile *spin_var;

int make_spin_pair(int fd[2]);
/*
inline int do_send_spin(int fd) {

	*spin_var = fd;
	__sync_synchronize();

	return 1;
}
inline int do_recv_spin(int fd) {

	while (*spin_var != fd) {
		__sync_synchronize();
	}

	return 1;
}
*/
int do_send_spin(int fd);
int do_recv_spin(int fd);

int cleanup_spin();

#endif
