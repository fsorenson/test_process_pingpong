#ifndef __UDP_H__
#define __UDP_H__

#include "comms.h"

//#include "test_process_pingpong.h"

int make_udp_pair(int fd[2]);
void comm_add_udp();

#endif
