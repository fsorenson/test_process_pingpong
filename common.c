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
#include <stdarg.h>


inline int get_min_stack_size(void) {
	long int minstack;

#ifdef _SC_THREAD_STACK_MIN
	if ((minstack = sysconf(_SC_THREAD_STACK_MIN)) == -1) {
		perror("sysconf(_SC_THREAD_STACK_MIN)");
		exit(EXIT_FAILURE);
	}
#else
	minstack = 16384;
#endif
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

inline volatile void *bytecopy(volatile void *const dest, volatile void const *const src, size_t bytes) {
	while (bytes-- > (size_t) 0)
		((volatile unsigned char *)dest)[bytes] = ((volatile unsigned char const *)src)[bytes];

	return dest;
}

int safe_write(int fd, char *buffer, int buffer_len, const char *fmt, ...) {
	va_list ap;
	int ret;
	int str_len;

	va_start(ap, fmt);
	ret = vsnprintf(buffer, buffer_len, fmt, ap);
	va_end(ap);

	str_len = strlen(buffer);
	if ((ret < 0) || (ret >= buffer_len) || (str_len >= buffer_len) || (ret != str_len)) {
		char out_buf[100];
		(void)snprintf(out_buf, 100, "Error with vsnprintf...  snprintf returned %d, but strlen is %d\n",
			ret, str_len);
		write(1, out_buf, strlen(out_buf));
	}

	ret = write(fd, buffer, str_len);
	if (ret < 0) {
		char out_buf[200];
		(void)snprintf(out_buf, 200, "Error with write(%d)...  expected to write %d: %s\n",
			fd, str_len, strerror(errno));
		(void)write(1, out_buf, strlen(out_buf));
	}

	return ret;
}

void exit_fail(const char *fmt, ...) {
#define OUTPUT_BUFFER_LEN 400
	static char output_buffer[OUTPUT_BUFFER_LEN];
	size_t output_buffer_len = OUTPUT_BUFFER_LEN;
#undef OUTPUT_BUFFER_LEN
	va_list ap;

	va_start(ap, fmt);
	safe_write(1, output_buffer, output_buffer_len, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}


int do_sleep(long sec, long nsec) {
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += sec;
	ts.tv_nsec += nsec;
	if (ts.tv_nsec >= 1000000000) {
		ts.tv_nsec -= 1000000000;
		ts.tv_sec ++;
	}
	clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, NULL);

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

	return	((long double)(end_tsc - start_tsc) / (long double)(end_time - start_time) / 1e3L / 1e3L);
}


inline void set_affinity(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET((size_t)cpu, &mask);

	if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1)
		exit_fail("Error setting affinity: %s\n", strerror(errno));
}

long double get_time(void) {
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return ((double) ts.tv_sec) + ((double)ts.tv_nsec / 1e9L);
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

struct timeval elapsed_time_timeval(const struct timeval start, const struct timeval stop) {
	struct timeval ret;

	if ((stop.tv_usec - start.tv_usec) < 0) {
		ret.tv_sec = stop.tv_sec - start.tv_sec - 1;
		ret.tv_usec = 1000000L + stop.tv_usec - start.tv_usec;
	} else {
		ret.tv_sec = stop.tv_sec - start.tv_sec;
		ret.tv_usec = stop.tv_usec - start.tv_usec;
	}
	return ret;
}

struct timeval str_to_timeval(const char *string) {
	struct timeval tv;
	char *ptr1, *ptr2;

	tv.tv_sec = strtol(string, &ptr1, 10);
	if ((tv.tv_sec == 0) && (ptr1 == string)) {
		printf("unable to parse '%s'\n", string);
		return tv;
	}

	if ((ptr1 == NULL) // apparently, it was all valid digits
		|| (ptr1[0] != '.')) // doesn't have a decimal point, for some reason
		return tv;

	tv.tv_usec = strtol(ptr1, &ptr2, 10);
	if ((tv.tv_usec == 0) && (ptr2 == ptr1)) {
		printf("unable to parse post-decimal-point portion of %s\n", string);
	}
	return tv;
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
                PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE,
		-1, 0);
}

integer_fixed_point_t __CONST f_to_fp(int prec, long double f) {
	integer_fixed_point_t int_fp;
	unsigned long mult;
	long long i;
	unsigned long long i2;
	long double fmult;

	fmult = powl(10.0L, prec * 1.0L);
	mult = (unsigned long)fmult;

	i = llroundl(f * fmult);
	i2 = (unsigned long long)((i < 0) ? (0 - i) : i);

	int_fp.prec = prec;

	int_fp.i = i2 / mult;
	int_fp.dec = i2 - (int_fp.i * mult);

	return int_fp;
}

// logb(x) = logc(x) / logc(b)
long double __CONST log_base(long double base, long double num) {
	return logl(num) / logl(base);
}

unsigned long long __CONST ld_to_ull(long double d) {
	return (unsigned long long)d;
}

long long __CONST ipowll(long x, long y) {
	double xf, yf;
	long double pf;

	xf = (double)x;
	yf = (double)y;
	pf = pow(xf, yf);

	return (long long)pf;
}

