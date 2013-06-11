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


#include "threads_children.h"

#include "signals.h"
#include "setup.h"
#include "sched.h"
#include "units.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

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

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, (struct timespec *)&run_data->thread_stats[thread_num].thread_time);

	if (config.set_affinity == true)
		run_data->thread_stats[thread_num].tsc = rdtsc(NULL);


	run_data->rusage_req[thread_num] = false;
	return 0;
}

static void read_sched_proc(int thread_num) {
	int fd;

	if ((fd = open(PROC_PID_SCHED_NAME, O_RDONLY)) < 0) {
		printf("Error opening %s: %s\n", PROC_PID_SCHED_NAME, strerror(errno));
		return;
	}
	if ((run_data->thread_stats[thread_num].sched_data_len
			= (int) read(fd, run_data->thread_stats[thread_num].sched_data, PROC_PID_SCHED_SIZE)) < 0) {
		printf("Unable to read thread %d stats from %s: %s\n", thread_num, PROC_PID_SCHED_NAME, strerror(errno));
	}
	close(fd);
//printf("Thread %d creating sched output file\n", thread_num);
	if (thread_num == 0)
		fd = open("/tmp/thread_sched.0", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	else
		fd = open("/tmp/thread_sched.1", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	write(fd, run_data->thread_stats[thread_num].sched_data, run_data->thread_stats[thread_num].sched_data_len);
	close(fd);
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

		read_sched_proc(thread_num);

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

static void setup_cpu_dma_latency(int thread_num) {
	unsigned long cpu_dma_latency_val;

	if (config.cpu_dma_latency == -1) /* no value configured */
		return;
	if (config.euid == 0) {
		if ((run_data->thread_info[thread_num].cpu_latency_fd = open(CPU_DMA_LATENCY_DEVICE, O_RDWR)) < 0) {
			printf("Error opening %s: %s\n", CPU_DMA_LATENCY_DEVICE, strerror(errno));

			return;
		}
		cpu_dma_latency_val = (unsigned long)config.cpu_dma_latency;
		write(run_data->thread_info[thread_num].cpu_latency_fd, &cpu_dma_latency_val, 4);
	}
}


void __NORETURN do_thread_work(int thread_num) {
#define OUTPUT_BUFFER_LEN 200
	char output_buffer[OUTPUT_BUFFER_LEN];
	size_t output_buffer_len = OUTPUT_BUFFER_LEN;
#undef OUTPUT_BUFFER_LEN

	rename_thread(run_data->thread_info[thread_num].thread_name);

	if (run_data->thread_info[thread_num].tid == 0)
		run_data->thread_info[thread_num].tid = (int)syscall(SYS_gettid);
	if (run_data->thread_info[thread_num].pid == 0)
		run_data->thread_info[thread_num].pid = (int)syscall(SYS_getpid);
	run_data->thread_info[thread_num].sid =
		(int)syscall(SYS_getsid, run_data->thread_info[thread_num].pid);
	run_data->thread_info[thread_num].pgid =
		(int)syscall(SYS_getpgid, run_data->thread_info[thread_num].pid);

	setup_cpu_dma_latency(thread_num);
//	estimate_cpu_speed(thread_num);


	snprintf(output_buffer, output_buffer_len, "%d: %s - thread %d, pid %d, tid %d, sid %d, pgid %d\n",
		thread_num, run_data->thread_info[thread_num].thread_name,
		run_data->thread_info[thread_num].thread_num,
		run_data->thread_info[thread_num].pid, run_data->thread_info[thread_num].tid,
		run_data->thread_info[thread_num].sid, run_data->thread_info[thread_num].pgid);
/*
	snprintf(output_buffer, output_buffer_len, "%d: %s - thread %d, pid %d, tid %d, sid %d, pgid %d, CPU estimated at %.2Lf MHz (%s cycle)\n",
		thread_num, run_data->thread_info[thread_num].thread_name,
		run_data->thread_info[thread_num].thread_num,
		run_data->thread_info[thread_num].pid, run_data->thread_info[thread_num].tid,
		run_data->thread_info[thread_num].sid, run_data->thread_info[thread_num].pgid,
		run_data->thread_info[thread_num].cpu_mhz,
		subsec_string(cpu_cycle_time_buffer, run_data->thread_info[thread_num].cpu_cycle_time, 3));
*/
	write(1, output_buffer, strlen(output_buffer));


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


