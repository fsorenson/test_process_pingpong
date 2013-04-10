#ifndef __UDP_H__
#define __UDP_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"

int make_udp_pair(int fd[2]);

#endif
