
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "threads.h"
#include "setup.h"
#include "comms.h"


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

static inline unsigned long long native_read_tscp(unsigned int *aux)
{
        unsigned long low, high;
        asm volatile(".byte 0x0f,0x01,0xf9"
                     : "=a" (low), "=d" (high), "=c" (*aux));
        return low | ((unsigned long long)high << 32);
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
	long double mhz[2]; /* approximate speed of the CPU */
};

static int estimate_cpu_speed(int thread_num) {
	long double start_time, end_time;
	unsigned long long start_tsc, end_tsc;
	long double interval_time;
	unsigned long long interval_tsc;

	start_tsc = rdtsc(NULL);
	start_time = get_time();
	do_sleep(1, 0);
	end_tsc = rdtsc(NULL);
	end_time = get_time();

	interval_time = end_time - start_time;
	interval_tsc = end_tsc - start_tsc;


	run_data->thread_info[thread_num].cpu_mhz = interval_tsc / interval_time / 1000 / 1000;
	run_data->thread_info[thread_num].cpu_cycle_time = interval_time / (long double)interval_tsc;

	return 0;
}

static void monitor_cleanup() {

	cleanup_comm_mode_info();
	free(run_data->thread_info[0].stack);
	free(run_data->thread_info[1].stack);

//	munmap(run_data);
}


void stop_handler(int signum) {
	static int visited = 0;
	(void)signum;

	if (visited) {
		printf("Trying to stop a second time?\n");
		return;
	}
	visited++;

	stop_timer();
	run_data->stop = true;

	kill(run_data->thread_info[0].pid, SIGUSR1);
	kill(run_data->thread_info[1].pid, SIGUSR2);

	printf("Waiting for child threads to die.  Die, kids!  Die!!!\n");
	while ((run_data->thread_info[0].pid != -1) || (run_data->thread_info[1].pid != -1)) {
		do_sleep(0, 5000000);
	}

	config.comm_cleanup();

	monitor_cleanup();

	exit(0);
}



static int gather_stats(struct interval_stats_struct *i_stats) {

	i_stats->current_count = run_data->ping_count;
	i_stats->current_time = get_time();

	run_data->rusage_req_in_progress = true;
	run_data->rusage_req[0] = true;
	run_data->rusage_req[1] = true;
	kill(run_data->thread_info[0].pid, SIGUSR1);
	kill(run_data->thread_info[1].pid, SIGUSR2);

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
		i_stats->mhz[0] = i_stats->interval_tsc[0] / i_stats->interval_time / 1000 / 1000;
		i_stats->mhz[1] = i_stats->interval_tsc[1] / i_stats->interval_time / 1000 / 1000;

		i_stats->cpi[0] = (long double)i_stats->interval_tsc[0] / (long double)i_stats->interval_count;
		i_stats->cpi[1] = (long double)i_stats->interval_tsc[1] / (long double)i_stats->interval_count;
	} else {
		i_stats->interval_tsc[0] = i_stats->interval_tsc[1] = 0;
		i_stats->mhz[0] = i_stats->mhz[1] = 0;
		i_stats->cpi[0] = i_stats->cpi[1] = 0;
	}

	i_stats->iteration_time = i_stats->interval_time / i_stats->interval_count;

	return 0;
}

void show_stats(int signum) {
	static char output_buffer[400];
	static char time_per_iteration_string[30];
	static char run_time_string[30];

	struct interval_stats_struct i_stats;

	(void)signum;

	memset(&i_stats, 0, sizeof(struct interval_stats_struct));

	if (run_data->rusage_req_in_progress == true) /* already waiting for results */
		return;
	gather_stats(&i_stats);

	if (run_data->stop == true) /* don't worry about output...  let's just pack up and go home */
		return;

	snprintf(output_buffer, 255, "%s - %llu iterations -> %s",
		subsec_string(run_time_string, i_stats.run_time, 3),
		i_stats.interval_count,
		subsec_string(time_per_iteration_string, i_stats.iteration_time, 3));

	output_buffer[255] = 0;
	write(1, output_buffer, strlen(output_buffer));



	snprintf(output_buffer, 256, "\t(ping %ld csw, %.2Lf cpi, %.3Lf GHz); (pong %ld csw, %.2Lf cpi, %.3Lf GHz)\n",
		i_stats.csw[0], i_stats.cpi[0], i_stats.mhz[0] / 1000.0,
		i_stats.csw[1], i_stats.cpi[1], i_stats.mhz[1] / 1000.0);


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

	if (i_stats.current_time >= run_data->timeout_time) {
		stop_handler(0);
		return;
	}
}

void child_handler(int signum) {
	pid_t pid;
	int status;

	(void)signum;


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


	setup_child_signals();
	setup_crash_handler();
	setup_stop_signal();

	sigfillset(&signal_mask);
	sigdelset(&signal_mask, SIGCHLD);
	sigdelset(&signal_mask, SIGALRM);
	sigdelset(&signal_mask, SIGINT);

	while ((run_data->ready[0] != true) || (run_data->ready[1] != true)) {
		do_sleep(0, 5000000);
	}

	setup_timer();
	__sync_synchronize();
	run_data->start = true; /* tell the child threads to begin */
	config.comm_begin();
	__sync_synchronize();

	run_data->start_time = run_data->last_stats_time = get_time();
	run_data->timeout_time = (double)run_data->start_time + (double)config.max_execution_time;
//	run_data->start_tsc = rdtsc(NULL);


	while (run_data->stop != true) {
		sigsuspend(&signal_mask);
	}
	config.comm_interrupt(SIGINT);
	stop_handler(1);

	return 0;
}

static int send_thread_stats(int thread_num) {
	if (getrusage(RUSAGE_THREAD, (struct rusage *)&run_data->thread_stats[thread_num].rusage) == -1) {
		perror("getting rusage");
	}

	if (config.set_affinity == true)
		run_data->thread_stats[thread_num].tsc = rdtsc(NULL);

	run_data->rusage_req[thread_num] = false;
	return 0;
}

static void interrupt_thread(int signum) {
	int thread_num;

	if (signum == SIGUSR1) {
		thread_num = 0;
	} else {
		thread_num = 1;
	}

	if (run_data->rusage_req[thread_num] == true)
		send_thread_stats(thread_num);
	if (run_data->stop == true) {
		config.comm_cleanup();
		exit(0);
	}
}
static int setup_interrupt_signal(int thread_num) {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = interrupt_thread;
	if (thread_num == 0)
		sigaction(SIGUSR1, &sa, NULL);
	else
		sigaction(SIGUSR2, &sa, NULL);

	return 0;
}


/*
static inline void __attribute__((hot)) __attribute__((optimize("-Ofast"))) do_ping_work(int thread_num) {
	while (1) {
		run_data->ping_count ++;

		while (config.comm_do_send(config.mouth[thread_num]) != 1);
		while (config.comm_do_recv(config.ear[thread_num]) != 1);
	}
}
static inline void __attribute__((hot)) __attribute__((optimize("-Ofast")))  do_pong_work(int thread_num) {
	while (1) {
		while (config.comm_do_recv(config.ear[thread_num]) != 1);
		while (config.comm_do_send(config.mouth[thread_num]) != 1);
	}
}
*/

static void do_thread_work(int thread_num) {
	char cpu_cycle_time_buffer[50];
	char buf[100];

	rename_thread(run_data->thread_info[thread_num].thread_name);

	if (run_data->thread_info[thread_num].tid == 0)
		run_data->thread_info[thread_num].tid = (int)syscall(SYS_gettid);
	if (run_data->thread_info[thread_num].pid == 0)
		run_data->thread_info[thread_num].pid = (int)syscall(SYS_getpid);

	estimate_cpu_speed(thread_num);


	snprintf(buf, 99, "%d: %s - thread %d, pid %d, tid %d, CPU estimated at %.2Lf MHz (%s cycle)\n",
		thread_num, run_data->thread_info[thread_num].thread_name,
		run_data->thread_info[thread_num].thread_num,
		run_data->thread_info[thread_num].pid, run_data->thread_info[thread_num].tid,
		run_data->thread_info[thread_num].cpu_mhz,
		subsec_string(cpu_cycle_time_buffer, run_data->thread_info[thread_num].cpu_cycle_time, 3));
	write(1, buf, strlen(buf));


	on_parent_death(SIGINT);
	setup_interrupt_signal(thread_num);
	setup_crash_handler();

	config.comm_pre();

	if (config.set_affinity == true) {
		set_affinity(config.cpu[thread_num]);
		sched_yield();

		run_data->thread_stats[thread_num].start_tsc = rdtsc(NULL);
		run_data->thread_stats[thread_num].last_tsc = run_data->thread_stats[thread_num].start_tsc;
	} else {
//		printf("Affinity not set, however pong *currently* running on cpu %d\n", sched_getcpu());
	}


	/* signal to the main thread that we're ready when they are */
	run_data->ready[thread_num] = true;

	while (run_data->start != true)
		;


	if (thread_num == 0) {
		config.comm_do_ping(thread_num);
	} else {
		config.comm_do_pong(thread_num);
	}


	config.comm_cleanup();
	exit(0);
}

static int thread_function(void *argument) {
	struct thread_info_struct *t_info = (struct thread_info_struct *)argument;

	do_thread_work(t_info->thread_num);
	return 0;
}

static void *pthread_function(void *argument) {

	thread_function(argument);

	return NULL;
}

static int do_fork() {
	int thread_num = 0;
	int pid;

	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "ping_thread", 12);
	run_data->stop = false;
	run_data->rusage_req_in_progress = false;
	run_data->rusage_req[0] = run_data->rusage_req[1] = false;

	pid = fork();
	if (pid == 0) { /* child proc */ /* ping */
		do_thread_work(thread_num);
	} else if (pid > 0) {
		run_data->thread_info[thread_num].pid = pid;

		thread_num ++;

		run_data->thread_info[thread_num].thread_num = thread_num;
		strncpy(run_data->thread_info[thread_num].thread_name, "pong_thread", 12);

		pid = fork();
		if (pid == 0) { /* child proc */ /* pong */
			do_thread_work(thread_num);
		} else if (pid > 0) {
			run_data->thread_info[thread_num].pid = pid;

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
static int do_clone() {
	int thread_num = 0;
	int clone_flags;

	run_data->thread_info[0].stack = calloc(1, STACK_SIZE); /* ping thread */
	run_data->thread_info[1].stack = calloc(1, STACK_SIZE); /* pong thread */

	if ((run_data->thread_info[0].stack == 0) || (run_data->thread_info[1].stack == 0)) {
		perror("calloc: could not allocate stack");
		exit(1);
	}


	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "ping_thread", 12);

//	clone_flags = CLONE_FS | CLONE_FILES | CLONE_PARENT | CLONE_VM | SIGCHLD;
	clone_flags = CLONE_FS | CLONE_FILES | SIGCHLD;

	run_data->thread_info[thread_num].tid = clone(&thread_function,
		(char *) run_data->thread_info[0].stack + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num],
		&run_data->thread_info[thread_num].ptid, NULL, &run_data->thread_info[thread_num].ctid
		);
//		(void *)&run_data->thread_info[thread_num]);

	thread_num++;

	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "pong_thread", 12);


	run_data->thread_info[thread_num].tid = clone(&thread_function,
		(char *) run_data->thread_info[1].stack + STACK_SIZE, clone_flags,
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

static int do_pthread() {
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
	strncpy(run_data->thread_info[thread_num].thread_name, "ping_thread", 12);

	if ((ret = pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num])) != 0) {
		errno = ret;
		printf("Error occurred while starting 'ping' thread: %m\n");
		exit(-1);
	}

	thread_num ++;

	run_data->thread_info[thread_num].thread_num = thread_num;
	strncpy(run_data->thread_info[thread_num].thread_name, "pong_thread", 12);

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

