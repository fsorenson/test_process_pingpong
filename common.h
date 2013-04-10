#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>

#define __PACKED	__attribute__ ((packed))
#define __NORETURN	__attribute__ ((noreturn))

typedef enum { no = 0, false = 0, yes = 1, true = 1 } __PACKED bool;

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define offsetof(type, member)  __builtin_offsetof (type, member)


int get_min_stack_size();
uint64_t rdtsc();
int do_sleep(long sec, long nsec);
void set_affinity(int cpu);
int rename_thread(char *thread_name);
void on_parent_death(int signum);

#endif
