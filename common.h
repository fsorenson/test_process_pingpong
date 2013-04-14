#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>

#define __PACKED	__attribute__ ((packed))
#define __NORETURN	__attribute__ ((noreturn))
#define __HOT		__attribute__((hot))

#define __OPTIMIZED0
#define __OPTIMIZED1	__attribute__((optimize("-Ofast")))
#define __OPTIMIZED	__OPTIMIZED1

#define __PINGPONG_FN  __NORETURN __HOT __OPTIMIZED


typedef enum { no = 0, false = 0, yes = 1, true = 1 } __PACKED bool;

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define offsetof(type, member)  __builtin_offsetof (type, member)


int get_min_stack_size();
uint64_t rdtsc();
long double get_time();
int do_sleep(long sec, long nsec);
long double estimate_cpu_mhz();
void set_affinity(int cpu);
int rename_thread(char *thread_name);
void on_parent_death(int signum);

#endif
