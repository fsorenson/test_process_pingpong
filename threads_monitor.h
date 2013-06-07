#ifndef __THREADS_MONITOR_H__
#define __THREADS_MONITOR_H__

#include "test_process_pingpong.h"
#include "signals.h"
#include "setup.h"

#include "units.h"
#include "sched.h"


/* thread startup, execution, handlers, etc */

void child_handler(int signum);

int do_monitor_work(void);

#endif
