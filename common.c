#include "common.h"

#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <time.h>


inline int get_min_stack_size() {
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

int do_sleep(long sec, long nsec) {
	struct timespec ts;

	ts.tv_sec = sec;
	ts.tv_nsec = nsec;
	clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);

	return 0;
}

long double estimate_cpu_mhz() {
	long double start_time, end_time;
	unsigned long long start_tsc, end_tsc;

	start_tsc = rdtsc(NULL);
	start_time = get_time();
	do_sleep(1, 0);
	end_tsc = rdtsc(NULL);
	end_time = get_time();

	return	((long double)(end_tsc - start_tsc) / (long double)(end_time - start_time) / 1000.0 / 1000.0);
}


inline void set_affinity(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET((size_t)cpu, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

long double get_time() {
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return ((double) ts.tv_sec) + ((double)ts.tv_nsec / 1000000000.0);
}

inline int rename_thread(char *thread_name) {
	char name[17];
	int ret;

	strncpy(name, thread_name, 16);
	name[16] = 0;
	if ((ret = prctl(PR_SET_NAME, (unsigned long)&name)) == -1) {
		printf("Failed to set thread name to '%s': %m\n", name);
		return -1;
	}
	return 0;
}

inline void on_parent_death(int signum) {
	int ret;

	if ((ret = prctl(PR_SET_PDEATHSIG, signum)) == -1) {
		perror("Failed to create message to be delivered upon my becoming an orphan: %m\n");
	}
}

