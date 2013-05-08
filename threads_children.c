#include "threads_children.h"
#include "test_process_pingpong.h"

#include "units.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/syscall.h>

static int  estimate_cpu_speed(int thread_num) {

	run_data->thread_info[thread_num].cpu_mhz = estimate_cpu_mhz();
	run_data->thread_info[thread_num].cpu_cycle_time = 1 / run_data->thread_info[thread_num].cpu_mhz / 1e6L;

	return 0;
}

static int send_thread_stats(int thread_num) {
#ifndef RUSAGE_THREAD
#define RUSAGE_THREAD 1
#endif
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

void __NORETURN do_thread_work(int thread_num) {
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

	config.comm_pre(thread_num);

	if (config.set_affinity == true) {
		set_affinity(config.cpu[thread_num]);
		sched_yield();

		run_data->thread_stats[thread_num].start_tsc = rdtsc(NULL);
		run_data->thread_stats[thread_num].last_tsc = run_data->thread_stats[thread_num].start_tsc;
	} else {
//              printf("Affinity not set, however pong *currently* running on cpu %d\n", sched_getcpu());
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

int __NORETURN thread_function(void *argument) {
	struct thread_info_struct *t_info = (struct thread_info_struct *)argument;

	do_thread_work(t_info->thread_num);
}

void * __NORETURN pthread_function(void *argument) {

	thread_function(argument);
}


