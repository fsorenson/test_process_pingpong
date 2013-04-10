#include "sched.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
		config.sched_rr_quantum = ((double)tp.tv_sec + (double)tp.tv_nsec) / 1000000000.0;
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
		config.sched_prio = (int)strtol(saveptr, NULL, 10);
	} else if (! strcmp(tok, "rr")) {
		config.sched_policy = SCHED_RR;
		config.sched_prio = (int)strtol(saveptr, NULL, 10);
	}

	return 0;
}



#ifndef HAVE_SCHED_GETCPU
#include <syscall.h>
#include <unistd.h>
//#include <asm-i386/unistd.h>
int sched_getcpu() {
        int c, s;
        s = (int)syscall(__NR_getcpu, &c, NULL, NULL);
        return (s == -1) ? s : c;
}
#endif

int num_cpus() {
	int ncpu;

	if ((ncpu = (int)sysconf(_SC_NPROCESSORS_CONF)) == -1) {
		perror("sysconf(_SC_NPROCESSORS_CONF)");
		exit(1);
	}
	return ncpu;
}
int num_online_cpus() {
	int ncpu;

	if ((ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN)) == -1) {
		perror("sysconf(_SC_NPROCESSORS_ONLN)");
		exit(1);
	}
	return ncpu;
}

