#ifndef __TCP_H__
#define __TCP_H__

#include "comms.h"


int new_make_tcp_pair(int fd[2]);

int make_tcp_pair(int fd[2]);
void comm_add_tcp();


#endif
