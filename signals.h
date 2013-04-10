#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include "test_process_pingpong.h"

long double get_time();

void print_backtrace(int signum);
void print_backtrace2(int signum);

int setup_timer();
void stop_timer();

int setup_stop_signal();
int setup_child_signals();
void setup_crash_handler();


#endif
