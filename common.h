/*
	This file is part of test_process_pingpong

	Copyright 2013 Frank Sorenson (frank@tuxrocks.com)


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <math.h>
#include <sys/types.h>

#define DEFAULT_OUTPUT_FD 1

#define __CONST		__attribute__((const))
#define __PURE		__attribute__((pure))
#define __PACKED	__attribute__((packed))
#define __NORETURN	__attribute__((noreturn))
#define __HOT		__attribute__((hot))

#ifndef OPTIMIZE_LEVEL
#define OPTIMIZE_LEVEL 3
#endif

#define OPTIMIZED3(x) #x
#define OPTIMIZED2(p1,p2) OPTIMIZED3(p1 ## p2)
#define __OPTIMIZED(OLEV)     __attribute__((optimize(OPTIMIZED2(-O, OLEV ))))

#define __PINGPONG_FN  __NORETURN __HOT __OPTIMIZED(OPTIMIZE_LEVEL)


typedef enum { no = 0, false = 0, yes = 1, true = 1 } __PACKED bool;

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#ifndef offsetof
#define offsetof(type, member)  __builtin_offsetof (type, member)
#endif

#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)/* 0666*/
#endif

#ifndef __GLIBC__
#define powl(x,y) \
	(double)pow( (double)x, (double)y)

#define log(x) \
	pow(M_E, x)
#endif

#define PERIOD .
#define BUILD_TIME_VAL(a,b,c) str(a) xstr(PERIOD) str(b) str(c)

#define xstr(s) str(s)
#define str(s) #s

#if __STDC_VERSION__ == 199901L
#define asm __asm__
#endif

// memory barriers
#define mb()    asm volatile("mfence":::"memory")
#define rmb()   asm volatile("lfence":::"memory")
#define wmb()   asm volatile("sfence"::: "memory")
#define mb2()	asm volatile ("" : : : "memory");
#define nop() __asm__ __volatile__ ("nop")

//__sync_synchronize()


int get_min_stack_size(void);
uint64_t rdtsc(unsigned int *aux);
long double get_time(void);
struct timespec elapsed_time(const struct timespec start, const struct timespec stop);
struct timeval elapsed_time_timeval(const struct timeval start, const struct timeval stop);

struct timeval str_to_timeval(const char *string);
volatile void *bytecopy(volatile void *const dest, volatile void const *const src, size_t bytes);
int safe_write(int fd, char *buffer, int buffer_len, const char *fmt, ...)
	__attribute__((format(printf, 4, 5) )) ;

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
