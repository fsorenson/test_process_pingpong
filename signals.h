#ifndef __SIGNALS_H__
#define __SIGNALS_H__

#include "test_process_pingpong.h"

void print_backtrace(int signum);
void print_backtrace_die(int signum);
void print_backtrace2(int signum);

int setup_timer(void);
void stop_timer(void);

int setup_stop_signal(void);
int setup_child_signals(void);
void setup_crash_handler(void);
int send_sig(int pid, int sig);


#endif
