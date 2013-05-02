#include "common.h"

#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <math.h>


inline int get_min_stack_size(void) {
	long int minstack;

	if ((minstack = sysconf(_SC_THREAD_STACK_MIN)) == -1) {
		perror("sysconf(_SC_THREAD_STACK_MIN)");
		exit(1);
	}
	return (int)minstack;
}

inline uint64_t rdtsc(unsigned int *aux) {
	uint32_t low, high;
	(void)aux;

	__asm__ __volatile__(   \
		"rdtsc"         \
		: "=a"(low),    \
		"=d"(high));  \


//	__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
	return (uint64_t) high << 32 | low;
}

static __inline__ __attribute__((always_inline)) unsigned long long __rdtsc(void)
{
	unsigned long long retval;
	__asm__ __volatile__("rdtsc" : "=A"(retval));
	return retval;
}

int do_sleep(long sec, long nsec) {
	struct timespec ts;

	ts.tv_sec = sec;
	ts.tv_nsec = nsec;
	clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);

	return 0;
}

long double estimate_cpu_mhz(void) {
	long double start_time, end_time;
	unsigned long long start_tsc, end_tsc;

	start_tsc = rdtsc(NULL);
	start_time = get_time();
	do_sleep(1, 0);
	end_tsc = rdtsc(NULL);
	end_time = get_time();

	return	((long double)(end_tsc - start_tsc) / (long double)(end_time - start_time) / 1e3 / 1e3);
}


inline void set_affinity(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET((size_t)cpu, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

long double get_time(void) {
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return ((double) ts.tv_sec) + ((double)ts.tv_nsec / 1e9);
}

struct timespec elapsed_time(const struct timespec start, const struct timespec stop) {
	struct timespec ret;

	if ((stop.tv_nsec - start.tv_nsec) < 0) {
		ret.tv_sec = stop.tv_sec - start.tv_sec - 1;
		ret.tv_nsec = 1000000000L + stop.tv_nsec - start.tv_nsec;
	} else {
		ret.tv_sec = stop.tv_sec - start.tv_sec;
		ret.tv_nsec = stop.tv_nsec - start.tv_nsec;
	}
	return ret;
}

inline int rename_thread(char *thread_name) {
	char name[17];
	int ret;

	strncpy(name, thread_name, 16);
	name[16] = 0;
	if ((ret = prctl(PR_SET_NAME, (unsigned long)&name)) == -1) {
		printf("Failed to set thread name to '%s': %s\n", name, strerror(errno));
		return -1;
	}
	return 0;
}

inline void on_parent_death(int signum) {
	int ret;

	if ((ret = prctl(PR_SET_PDEATHSIG, signum)) == -1) {
		printf("Failed to create message to be delivered upon my becoming an orphan: %s\n", strerror(errno));
	}
}

int init_mlockall(void) {

	if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
		printf("Failed to lock memory...paging latencies may occur.  Error was '%s'\n", strerror(errno));
		return 1;
	}

	return 0;
}

unsigned int __CONST page_align_size(unsigned int len, int size_align_flag) {
	unsigned int page_size;

	switch (size_align_flag) {
		case SIZE_ALIGN_NONE:
			return len;
			break;
		case SIZE_ALIGN_NORMAL:
			page_size = 4096;
			break;
		case SIZE_ALIGN_HUGE:
		default:
			page_size = 2*1024*1024;
			break;
	};

	return (len % page_size == 0) ? len : ((len / page_size) + 1) * page_size;
}

void *map_shared_area(unsigned int len, int size_align_flag) {
	return mmap(NULL, page_align_size(len, size_align_flag),
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

integer_fixed_point_t __CONST f_to_fp(int prec, long double f) {
	integer_fixed_point_t int_fp;
	unsigned long mult;
	long long i;
	unsigned long long i2;
	long double fmult;

	fmult = pow(10.0, prec * 1.0);
	mult = (unsigned long)fmult;

	i = llroundl(f * fmult);
	i2 = (unsigned long long)((i < 0) ? (0 - i) : i);

	int_fp.prec = prec;

	int_fp.i = i2 / mult;
	int_fp.dec = i2 - (int_fp.i * mult);

	return int_fp;
}
}
}

