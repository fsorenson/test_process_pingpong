#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>

#define __CONST		__attribute__((const))
#define __PURE		__attribute__((pure))
#define __PACKED	__attribute__((packed))
#define __NORETURN	__attribute__((noreturn))
#define __HOT		__attribute__((hot))

#define __OPTIMIZED0
#define __OPTIMIZED1	__attribute__((optimize("-Ofast")))
#define __OPTIMIZED	__OPTIMIZED0

#define __PINGPONG_FN  __NORETURN __HOT __OPTIMIZED1


typedef enum { no = 0, false = 0, yes = 1, true = 1 } __PACKED bool;

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define offsetof(type, member)  __builtin_offsetof (type, member)

#define xstr(s) str(s)
#define str(s) #s


// memory barriers
#define mb()    asm volatile("mfence":::"memory")
#define rmb()   asm volatile("lfence":::"memory")
#define wmb()   asm volatile("sfence"::: "memory")


int get_min_stack_size(void);
uint64_t rdtsc(unsigned int *aux);
long double get_time(void);
struct timespec elapsed_time(const struct timespec start, const struct timespec stop);
int do_sleep(long sec, long nsec);
long double estimate_cpu_mhz(void);
void set_affinity(int cpu);
int rename_thread(char *thread_name);
void on_parent_death(int signum);
int init_mlockall(void);


#define SIZE_ALIGN_NONE         0
#define SIZE_ALIGN_NORMAL       1
#define SIZE_ALIGN_HUGE         2

unsigned int page_align_size(unsigned int len, int size_align_flag);
void *map_shared_area(unsigned int len, int size_align_flag);


typedef struct integer_fixed_point {
	unsigned long int i;
	unsigned long int dec;
	int prec;
	char dummy[4];
} integer_fixed_point_t;

integer_fixed_point_t __CONST f_to_fp(int prec, long double f);

long double log_base(long double base, long double num);
unsigned long long ld_to_ull(long double d);
long long ipowll(long x, long y);

#endif
