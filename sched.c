#include "sched.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef SET_PRIORITIES
int get_priorities() {
	struct sched_param param;

	config.cur_sched_policy = sched_getscheduler(0);
	sched_getparam(0, &param);
	config.cur_sched_prio = param.sched_priority;

	return 0;
}
int set_priorities() {
	struct sched_param param;

	if (config.euid == 0) {
		param.sched_priority = config.sched_prio;
		if (sched_setscheduler(getpid(), config.sched_policy, &param)){
			perror("Error setting scheduler\n");
		}
	}

	return 0;
}
int get_sched_interval() {
	struct timespec tp;
	int ret;

	if ((ret = sched_rr_get_interval(0, &tp)) == -1) {
		perror("erro getting RR scheduling interval: %m\n");
	} else {
		config.sched_rr_quantum = (tp.tv_sec * 1.0) + (tp.tv_nsec * 1.0) / 1000000000;
	}

	return 0;
}
int parse_sched(char *arg) {
	char *arg_copy;
	char *tok, *saveptr;

	arg_copy = strdup(arg);
	tok = strtok_r(arg_copy, ":", &saveptr);
	if (! strcmp(tok, "fifo")) {
		config.sched_policy = SCHED_FIFO;
		config.sched_prio = strtol(saveptr, NULL, 10);
	} else if (! strcmp(tok, "rr")) {
		config.sched_policy = SCHED_RR;
		config.sched_prio = strtol(saveptr, NULL, 10);
	}

	return 0;
}
#endif


#ifndef HAVE_SCHED_GETCPU
#include <syscall.h>
#include <unistd.h>
#include <asm-i386/unistd.h>
int sched_getcpu() {
        int c, s;
        s = syscall(__NR_getcpu, &c, NULL, NULL);
        return (s == -1) ? s : c;
}
#else
int sched_getcpu() {
        return -1;
}
#endif
int num_cpus() {
	int ncpu;

	if ((ncpu = sysconf(_SC_NPROCESSORS_CONF)) == -1) {
		perror("sysconf(_SC_NPROCESSORS_CONF)");
		exit(1);
	}
	return ncpu;
}
int num_online_cpus() {
	int ncpu;

	if ((ncpu = sysconf(_SC_NPROCESSORS_ONLN)) == -1) {
		perror("sysconf(_SC_NPROCESSORS_ONLN)");
		exit(1);
	}
	return ncpu;
}

