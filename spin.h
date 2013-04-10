#ifndef __SPIN_H__
#define __SPIN_H__

#include "comms.h"

int make_spin_pair(int fd[2]);

int do_send_spin(int fd);
int do_recv_spin(int fd);

int cleanup_spin();

void comm_add_spin();

#endif
