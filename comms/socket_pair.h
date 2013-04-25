#ifndef __SOCKET_PAIR_H__
#define __SOCKET_PAIR_H__

#include "comms.h"

int make_socket_pair(int fd[2]);
void comm_add_socket_pair(void);

#endif
