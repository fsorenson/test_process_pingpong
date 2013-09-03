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


#include "sched.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int get_priorities(void) {
	struct sched_param param;

	config.cur_sched_policy = sched_getscheduler(0);
	sched_getparam(0, &param);
	config.cur_sched_prio = param.sched_priority;

	return 0;
}
int set_priorities(void) {
	struct sched_param param;

	if (config.euid == 0) {
		param.sched_priority = config.sched_prio;
		if (sched_setscheduler(getpid(), config.sched_policy, &param)){
			perror("Error setting scheduler\n");
		}
	}

	return 0;
}
int get_sched_interval(void) {
	struct timespec tp;
	int ret;

	if ((ret = sched_rr_get_interval(0, &tp)) == -1) {
		perror("erro getting RR scheduling interval: %m\n");
	} else {
		config.sched_rr_quantum = ((long double)tp.tv_sec + (long double)tp.tv_nsec) / 1e9L;
	}

	return 0;
}
int parse_sched(char *arg) {
	char *arg_copy;
	char *tok, *saveptr;

	arg_copy = strdup(arg);
	tok = strtok_r(arg_copy, ":", &saveptr);
	if (strcmp(tok, "fifo") == 0) {
		config.sched_policy = SCHED_FIFO;
		config.sched_prio = (int)strtol(saveptr, NULL, 10);
	} else if (strcmp(tok, "rr") == 0) {
		config.sched_policy = SCHED_RR;
		config.sched_prio = (int)strtol(saveptr, NULL, 10);
	}

	return 0;
}



#ifndef HAVE_SCHED_GETCPU
#include <syscall.h>
#include <unistd.h>
//#include <asm-i386/unistd.h>
int sched_getcpu(void) {
        int c, s;
        s = (int)syscall(__NR_getcpu, &c, NULL, NULL);
        return (s == -1) ? s : c;
}
#endif

int num_cpus(void) {
	int ncpu;

	if ((ncpu = (int)sysconf(_SC_NPROCESSORS_CONF)) == -1)
		exit_fail("sysconf(_SC_NPROCESSORS_CONF): error: %s", strerror(errno));
	return ncpu;
}
int num_online_cpus(void) {
	int ncpu;

	if ((ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN)) == -1)
		exit_fail("sysconf(_SC_NPROCESSORS_ONLN): error: %s", strerror(errno));
	return ncpu;
}

