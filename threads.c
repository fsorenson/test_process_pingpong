
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "threads.h"
#include "setup.h"


#include "units.h"
#include "sched.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/types.h>

/* thread startup, execution, handlers, etc */

static inline uint64_t rdtsc(unsigned int *aux) {
(void)aux;
	uint32_t low, high;

	__asm__ __volatile__(	\
		"rdtsc"		\
		: "=a"(low),	\
		  "=d"(high));	\


//	__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
	return (uint64_t) high << 32 | low;
}
static inline unsigned long long native_read_tscp(unsigned int *aux)
{
        unsigned long low, high;
        asm volatile(".byte 0x0f,0x01,0xf9"
                     : "=a" (low), "=d" (high), "=c" (*aux));
        return low | ((unsigned long long)high << 32);
}


void set_affinity(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET((size_t)cpu, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

int rename_thread(char *thread_name) {
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

struct interval_stats_struct {
	unsigned long long current_count;
	unsigned long long interval_count;
	long double current_time;
	long double interval_time;
	long double run_time;
	struct rusage rusage[2];
	unsigned long long interval_tsc[2];

	long csw[2]; /* per-thread per-interval csw */

	long double iteration_time; /* time for each iteration */
	long double cpi[2]; /* cycles-per-iteration per CPU */
};


int gather_stats(struct interval_stats_struct *i_stats) {

	i_stats->current_count = run_data->ping_count;
	i_stats->current_time = get_time();

	run_data->rusage_req_in_progress = true;
	run_data->rusage_req[0] = true;
	run_data->rusage_req[1] = true;


	i_stats->run_time = i_stats->current_time - run_data->start_time;
	i_stats->interval_time = i_stats->current_time - run_data->last_stats_time;
	i_stats->interval_count = i_stats->current_count - run_data->last_ping_count;


	while (run_data->rusage_req_in_progress == true) {
		if ((run_data->rusage_req[0] == false) && (run_data->rusage_req[1] == false))
			run_data->rusage_req_in_progress = false;
		if (run_data->rusage_req_in_progress == true)
			__sync_synchronize();
		if (run_data->stop == true)
			return -1;
	}

	i_stats->rusage[0].ru_nvcsw = run_data->thread_stats[0].rusage.ru_nvcsw - run_data->thread_stats[0].last_rusage.ru_nvcsw;
	i_stats->rusage[0].ru_nivcsw = run_data->thread_stats[0].rusage.ru_nivcsw - run_data->thread_stats[0].last_rusage.ru_nivcsw;
	i_stats->rusage[1].ru_nvcsw = run_data->thread_stats[1].rusage.ru_nvcsw - run_data->thread_stats[1].last_rusage.ru_nvcsw;
	i_stats->rusage[1].ru_nivcsw = run_data->thread_stats[1].rusage.ru_nivcsw - run_data->thread_stats[1].last_rusage.ru_nivcsw;

	i_stats->csw[0] = i_stats->rusage[0].ru_nvcsw + i_stats->rusage[0].ru_nivcsw;
	i_stats->csw[1] = i_stats->rusage[1].ru_nvcsw + i_stats->rusage[1].ru_nivcsw;


	if (config.set_affinity == true) {
		i_stats->interval_tsc[0] = run_data->thread_stats[0].tsc - run_data->thread_stats[0].last_tsc;
		i_stats->interval_tsc[1] = run_data->thread_stats[1].tsc - run_data->thread_stats[1].last_tsc;

		i_stats->cpi[0] = (long double)i_stats->interval_tsc[0] / (long double)i_stats->interval_count;
		i_stats->cpi[1] = (long double)i_stats->interval_tsc[1] / (long double)i_stats->interval_count;
	}

	i_stats->iteration_time = i_stats->interval_time / i_stats->interval_count;

	return 0;
}

void show_stats(int signum) {
	(void)signum;

	static char output_buffer[400];
	static char time_per_iteration_string[30];
	static char cycles_per_iteration_string[2][30];
	static char run_time_string[30];


	struct interval_stats_struct i_stats;

	if (run_data->rusage_req_in_progress == true) /* already waiting for results */
		return;
	gather_stats(&i_stats);

	if (run_data->stop == true) /* don't worry about output...  let's just pack up and go home */
		return;
	if (i_stats.current_time >= run_data->timeout_time) /* let our children start to die off while we finish things up */
		run_data->stop = true;

	snprintf(output_buffer, 255, "%s - %llu iterations -> %s",
		subsec_string(run_time_string, i_stats.run_time, 3),
		i_stats.interval_count,
		subsec_string(time_per_iteration_string, i_stats.iteration_time, 3));

	output_buffer[255] = 0;
	write(1, output_buffer, strlen(output_buffer));



	snprintf(output_buffer, 256, "\t(ping %ld csw, %Lf cpi); (pong %ld csw, %Lf cpi)\n",
		i_stats.csw[0], i_stats.cpi[0],
		i_stats.csw[1], i_stats.cpi[1]);

/*
	snprintf(output_buffer, 256, "\tping %ldcsw, %s cpi; pong %ldcsw, %s cpi\n",
		csw[0], subsec_string(cycles_per_iteration_string[0], cpi[0], 3),
		csw[1], subsec_string(cycles_per_iteration_string[1], cpi[1], 3));
*/

/*
	snprintf(output_buffer, 255, "\tping csw: vol=%ld, invol=%ld; pong csw: vol=%ld, invol=%ld\n",
		i_stats.rusage[0].ru_nvcsw, i_stats.rusage[0].ru_nivcsw,
		i_stats.rusage[1].ru_nvcsw, i_stats.rusage[1].ru_nivcsw);
*/


//	snprintf(buffer, 255, "context switches: vol=%ld, invol=%ld\n", usage.ru_nvcsw, usage.ru_nivcsw);
	output_buffer[255] = 0;
	write(1, output_buffer, strlen(output_buffer));

	/* cleanup things for the next time we come back */
	memcpy((void *)&run_data->thread_stats[0].last_rusage, (void *)&run_data->thread_stats[0].rusage, sizeof(struct rusage));
	memcpy((void *)&run_data->thread_stats[1].last_rusage, (void *)&run_data->thread_stats[1].rusage, sizeof(struct rusage));

	run_data->last_ping_count = i_stats.current_count;
	run_data->last_stats_time = i_stats.current_time;
	run_data->thread_stats[0].last_tsc = run_data->thread_stats[0].tsc;
	run_data->thread_stats[1].last_tsc = run_data->thread_stats[1].tsc;

}

void stop_handler(int signum) {
	(void)signum;

	stop_timer();
	run_data->stop = true;
}

void child_handler(int signum) {
	(void)signum;
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
		if (pid == run_data->thread_info[0].pid) {
			run_data->thread_info[0].pid = -1;
			run_data->stop = true;

			stop_timer();
		} else if (pid == run_data->thread_info[1].pid) {
			run_data->thread_info[1].pid = -1;
			run_data->stop = true;

			stop_timer();
		} else if (pid == 0) { /* no more pids have been reported dead */
			break;
		} else {
			printf("Got a SIGCHLD from pid '%d', but...\n", pid);
			printf("\tBilly Jean is not my lover...\n");
			printf("\tshe's just a girl who says that I am the one...\n");
			printf("\tbut, the kid is not my son...\n");
			/* wtf? */
		}
	} /* no more processes have died */
}

int do_monitor_work() {
	sigset_t signal_mask;

	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGCHLD);
	sigdelset(&signal_mask, SIGALRM);
	sigdelset(&signal_mask, SIGINT);

	setup_child_signals();
	setup_crash_handler();

	setup_timer();
	setup_stop_signal();
	run_data->start_time = run_data->last_stats_time = get_time();
	run_data->timeout_time = (double)run_data->start_time + (double)config.max_execution_time;
//	run_data->start_tsc = rdtsc(NULL);

	while (run_data->stop != true) {
		sigsuspend(&signal_mask);
	}
	config.comm_interrupt();

	printf("Waiting for child threads to die.  Die, kids!  Die!!!\n");
	while ((run_data->thread_info[0].pid != -1) || (run_data->thread_info[1].pid != -1)) {
		sigsuspend(&signal_mask);
	}

	config.comm_cleanup();
	return 0;
}

/* signal to receive if my parent process dies */
void on_parent_death(int signum) {
	int ret;

	if ((ret = prctl(PR_SET_PDEATHSIG, signum)) == -1) {
		perror("Failed to create message to be delivered upon my becoming an orphan: %m\n");
	}
}

int send_thread_stats(int thread_num) {
//printf("getting stats for %d\n", thread_num);
	if (getrusage(RUSAGE_THREAD, (struct rusage *)&run_data->thread_stats[thread_num].rusage) == -1) {
		perror("getting rusage");
	}

	run_data->thread_stats[thread_num].tsc = rdtsc(NULL);

	run_data->rusage_req[thread_num] = false;
//printf("Done getting stats for %d\n", thread_num);
	return 0;
}

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define CHECK_NEED_STATS(thr_num) \
	if (unlikely(run_data->rusage_req[thr_num] == true)) \
		send_thread_stats(thr_num);

#define SEND_BLOCK(thr_num) \
	while ((config.do_send(config.mouth[thr_num]) != 1) && unlikely(run_data->stop != true)) \
		CHECK_NEED_STATS((thr_num))

#define RECV_BLOCK(thr_num) \
	while ((config.do_recv(config.ear[thr_num]) != 1) && unlikely(run_data->stop != true)) \
		CHECK_NEED_STATS((thr_num))

inline void do_ping_work(int thread_num) {
		while (run_data->stop != true) {
			run_data->ping_count ++;

			SEND_BLOCK(thread_num);

			RECV_BLOCK(thread_num);

			CHECK_NEED_STATS(thread_num);

		}
}
void do_thread_work(int thread_num) {

	rename_thread(run_data->thread_info[thread_num].thread_name);
//write(1, "thread\n", 6);
//printf("thread %d here, pid %d is my name\n", thread_num, run_data->thread_info[thread_num]->pid);
//printf("tid = %ld\n", syscall(SYS_gettid));
	on_parent_death(SIGINT);
	setup_crash_handler();

	config.pre_comm();

	if (config.set_affinity == true) {
		set_affinity(config.cpu[thread_num]);
		sched_yield();
		run_data->thread_stats[thread_num].start_tsc = rdtsc(NULL);
		run_data->thread_stats[thread_num].last_tsc = run_data->thread_stats[thread_num].start_tsc;
	} else {
//		printf("Affinity not set, however pong *currently* running on cpu %d\n", sched_getcpu());
	}


	if (thread_num == 0) {
		while (run_data->stop != true) {
			run_data->ping_count ++;

			SEND_BLOCK(thread_num);
			RECV_BLOCK(thread_num);
			CHECK_NEED_STATS(thread_num);
		}
	} else {
		while (run_data->stop != true) {
			RECV_BLOCK(thread_num);
			SEND_BLOCK(thread_num);
			CHECK_NEED_STATS(thread_num);
		}
	} /* while run_data->stop != true */


	config.comm_cleanup();
	exit(0);
}

int thread_function(void *argument) {
	struct thread_info_struct *t_info = (struct thread_info_struct *)argument;
	int thread_num = t_info->thread_num;

	char buf[100];

	if (run_data->thread_info[thread_num].tid == 0)
		run_data->thread_info[thread_num].tid = (int)syscall(SYS_gettid);
	if (run_data->thread_info[thread_num].pid == 0)
		run_data->thread_info[thread_num].pid = (int)syscall(SYS_getpid);

	snprintf(buf, 99, "%d: %s - thread %d, pid %d, tid %d\n", thread_num, t_info->thread_name,  t_info->thread_num, t_info->pid, t_info->tid);
	write(1, buf, strlen(buf));

	do_thread_work(t_info->thread_num);
	return 0;
}

void *pthread_function(void *argument) {


	thread_function(argument);

	return NULL;
}

int do_fork() {
	int thread_num = 0;
	int pid;

	run_data->thread_info[thread_num].thread_num = thread_num;
	run_data->thread_info[thread_num].thread_name = "ping_thread";
	run_data->stop = false;
	run_data->rusage_req_in_progress = false;
	run_data->rusage_req[0] = run_data->rusage_req[1] = false;

	pid = fork();
	if (pid == 0) { /* child proc */ /* ping */
		do_thread_work(thread_num);
	} else if (pid > 0) {
		run_data->thread_info[thread_num].pid = pid;

//printf("forked %d\n", thread_info[thread_num].pid);

		thread_num ++;

		run_data->thread_info[thread_num].thread_num = thread_num;
		run_data->thread_info[thread_num].thread_name = "pong_thread";

		pid = fork();
		if (pid == 0) { /* child proc */ /* pong */
			do_thread_work(thread_num);
		} else if (pid > 0) {
			run_data->thread_info[thread_num].pid = pid;

//printf("forked %d\n", thread_info[thread_num].pid);

			do_monitor_work();
		} else {
			printf("Error while trying to fork: %d: %m\n", run_data->thread_info[thread_num].pid);
			exit(-1);
		}
	} else {
		printf("Error while trying to fork: %d: %m\n", run_data->thread_info[thread_num].pid);
		exit(-1);
	}

	return 0;
}
int do_clone() {
	int thread_num = 0;
	int clone_flags;

	config.stack[0] = calloc(1, STACK_SIZE); /* ping thread */
	config.stack[1] = calloc(1, STACK_SIZE); /* pong thread */

	if ((config.stack[0] == 0) || (config.stack[1] == 0)) {
		perror("calloc: could not allocate stack");
		exit(1);
	}


	run_data->thread_info[thread_num].thread_num = thread_num;
	run_data->thread_info[thread_num].thread_name = "ping_thread";

//	clone_flags = CLONE_FS | CLONE_FILES | CLONE_PARENT | CLONE_VM | SIGCHLD;
	clone_flags = CLONE_FS | CLONE_FILES | SIGCHLD;

	run_data->thread_info[thread_num].thread_id = clone(&thread_function,
		(char *) config.stack[0] + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num]);

	thread_num++;

	run_data->thread_info[thread_num].thread_num = thread_num;
	run_data->thread_info[thread_num].thread_name = "pong_thread";


	run_data->thread_info[thread_num].thread_id = clone(&thread_function,
		(char *) config.stack[1] + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num]);
	/* what about CLONE_SIGHAND and CLONE_VM ? */



	if ((run_data->thread_info[0].pid == -1) || (run_data->thread_info[1].pid == -1)) {
		perror("clone");
		exit(2);
	}
	do_monitor_work();
//	free(config.stack[1]);
//	thread_info->pid = syscall(SYS_getpid);

	return 0;
}

int do_pthread() {
	int thread_num = 0;
	pthread_attr_t attr;
	int ret;

	if ((ret = pthread_attr_init(&attr)) != 0) {
		errno = ret; perror("pthread_attr_init"); exit(-1);
	}
	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		errno = ret ; perror("pthread_attr_setdetachstate"); exit(-1);
	}

	run_data->thread_info[thread_num].thread_num = thread_num;
	run_data->thread_info[thread_num].thread_name = "ping_thread";

	pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num]); thread_num ++;


	run_data->thread_info[thread_num].thread_num = thread_num;
	run_data->thread_info[thread_num].thread_name = "pong_thread";

	pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num]);


	if ((ret = pthread_attr_destroy(&attr)) != 0) {
		errno = ret; perror("pthread_attr_destroy"); exit(-1);
	}

	do_monitor_work();

	return 0;
}

int start_threads() {
	printf("parent process is %d\n", getpid());

	if (config.thread_mode == thread_mode_fork) {
		do_fork();
	} else if (config.thread_mode == thread_mode_thread) {
		do_clone();
	} else if (config.thread_mode == thread_mode_pthread) {
		do_pthread();
	} else {
		printf("No thread mode specified?\n");
		exit(-1);
	}

	return 0;
}

