
#include "threads_monitor.h"
#include "threads_children.h"
#include "stats_final.h"
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
#include <sys/mman.h>



/* thread startup, execution, handlers, etc */

static inline unsigned long long native_read_tscp(unsigned int *aux)
{
        unsigned long low, high;
        asm volatile(".byte 0x0f,0x01,0xf9"
                     : "=a" (low), "=d" (high), "=c" (*aux));
        return low | ((unsigned long long)high << 32);
}


static void monitor_cleanup(void) {
	free(run_data->thread_info[0].stack);
	free(run_data->thread_info[1].stack);

	munmap(run_data, sizeof(struct run_data_struct));
}

static void stop_threads(void) {
	run_data->stop = true;
	mb();

	send_sig(run_data->thread_info[0].pid, SIGUSR1);
	send_sig(run_data->thread_info[1].pid, SIGUSR2);
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
	stop_threads();

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
	send_sig(run_data->thread_info[0].pid, SIGUSR1);
	send_sig(run_data->thread_info[1].pid, SIGUSR2);

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

	i_stats->rusage[0].ru_utime = elapsed_time_timeval(run_data->thread_stats[0].last_rusage.ru_utime, run_data->thread_stats[0].rusage.ru_utime);
	i_stats->rusage[0].ru_stime = elapsed_time_timeval(run_data->thread_stats[0].last_rusage.ru_stime, run_data->thread_stats[0].rusage.ru_stime);

	i_stats->rusage[1].ru_utime = elapsed_time_timeval(run_data->thread_stats[1].last_rusage.ru_utime, run_data->thread_stats[1].rusage.ru_utime);
	i_stats->rusage[1].ru_stime = elapsed_time_timeval(run_data->thread_stats[1].last_rusage.ru_stime, run_data->thread_stats[1].rusage.ru_stime);

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


void show_periodic_stats(int signum) {
	static struct interval_stats_struct i_stats = { 0 };
	(void)signum;

	memset(&i_stats, 0, sizeof(struct interval_stats_struct));

	if (run_data->rusage_req_in_progress == true) /* already waiting for results */
		return;
	gather_stats(&i_stats);

	if (run_data->stop == true) /* don't worry about output...  let's just pack up and go home */
		return;

	if (run_data->stats_headers_count++ % config.stats_headers_frequency == 0)
		show_stats_header();

	show_stats(&i_stats);
	store_last_stats(&i_stats);
}

void show_stats_header(void) {
#define OUTPUT_BUFFER_LEN 400
	static char output_buffer[OUTPUT_BUFFER_LEN];
	size_t output_buffer_len = OUTPUT_BUFFER_LEN;
#undef OUTPUT_BUFFER_LEN

	// original format
	// 83.000 s - 64962 iterations -> 15.393 us, ping: 40945.64 cycles, pong: 40945.100 cycles

	// header
	snprintf(output_buffer, output_buffer_len,
		"%7s %12s %11s", "", "", "");
	snprintf(output_buffer + strlen(output_buffer), output_buffer_len - strlen(output_buffer),
		" %s %s",
		" ___________ PING ___________ ",
		" ___________ PONG ___________ "
		);
	write(1, output_buffer, strlen(output_buffer));
	write(1, "\n", 1);

	snprintf(output_buffer, output_buffer_len,
		"%7s %12s %11s",
		"time", "cycles/sec", "cycle time");

	// per-thread header
	snprintf(output_buffer + strlen(output_buffer), output_buffer_len - strlen(output_buffer),
		" | %13s  %5s  %5s",
		" vol/inv csw", "user", "sys");
	snprintf(output_buffer + strlen(output_buffer), output_buffer_len - strlen(output_buffer),
		" | %13s  %5s  %5s",
		" vol/inv csw", "user", "sys");
	write(1, output_buffer, strlen(output_buffer));

	write(1, "\n", 1);
}

void show_stats(struct interval_stats_struct *i_stats) {
#define OUTPUT_BUFFER_LEN 400
	static char output_buffer[OUTPUT_BUFFER_LEN] = { 0 };
	size_t output_buffer_len = OUTPUT_BUFFER_LEN;
#undef OUTPUT_BUFFER_LEN
#define TEMP_STRING_LEN 30
	static char temp_string1[TEMP_STRING_LEN] = { 0 };
	static char temp_string2[TEMP_STRING_LEN] = { 0 };
	size_t temp_string_len = TEMP_STRING_LEN;
#undef TEMP_STRING_LEN
	integer_fixed_point_t int_fp1, int_fp2;

	memset(output_buffer, 0, output_buffer_len);
	memset(temp_string1, 0, temp_string_len);
	memset(temp_string2, 0, temp_string_len);

	// general stats
	snprintf(output_buffer, output_buffer_len, "%7s %12llu %11s",
		subsec_string(temp_string1, i_stats->run_time, 1),
		i_stats->interval_count,
		subsec_string(temp_string2, i_stats->iteration_time, 2));
	write(1, output_buffer, strlen(output_buffer));

	// per-thread stats
	snprintf(output_buffer, output_buffer_len, " | %5ld / %5ld",
		i_stats->rusage[0].ru_nvcsw, i_stats->rusage[0].ru_nivcsw);
	write(1, output_buffer, strlen(output_buffer));

	int_fp1 = f_to_fp(1, ((i_stats->rusage[0].ru_utime.tv_sec * 100.0L) + (i_stats->rusage[0].ru_utime.tv_usec / 1.0e4L) / i_stats->interval_time));
	int_fp2 = f_to_fp(1, ((i_stats->rusage[0].ru_stime.tv_sec * 100.0L) + (i_stats->rusage[0].ru_stime.tv_usec / 1.0e4L) / i_stats->interval_time));

	snprintf(output_buffer, output_buffer_len, "  %2lu.%01lu%%  %2lu.%01lu%%",
		int_fp1.i, int_fp1.dec, int_fp2.i, int_fp2.dec);
	write(1, output_buffer, strlen(output_buffer));

	snprintf(output_buffer, output_buffer_len, " | %5ld / %5ld",
		i_stats->rusage[1].ru_nvcsw, i_stats->rusage[1].ru_nivcsw);
	write(1, output_buffer, strlen(output_buffer));

	int_fp1 = f_to_fp(1, ((i_stats->rusage[1].ru_utime.tv_sec * 100.0L) + (i_stats->rusage[1].ru_utime.tv_usec / 1.0e4L) / i_stats->interval_time));
	int_fp2 = f_to_fp(2, ((i_stats->rusage[1].ru_stime.tv_sec * 100.0L) + (i_stats->rusage[1].ru_stime.tv_usec / 1.0e4L) / i_stats->interval_time));

	snprintf(output_buffer, output_buffer_len, "  %2lu.%01lu%%  %2lu.%01lu%%",
		int_fp1.i, int_fp1.dec, int_fp2.i, int_fp2.dec);
	write(1, output_buffer, strlen(output_buffer));

/* cpu cycles/pingpong for ping
	if (config.set_affinity == true) {
		snprintf(output_buffer, output_buffer_len, ", ping: %d.%d cyc.",
			(int)i_stats->cpi[0], (int)(i_stats->cpi[0] * 100.0L) % 100);
		write(1, output_buffer, strlen(output_buffer));
		snprintf(output_buffer, output_buffer_len, ", %ld/%ld csw",
			i_stats->rusage[0].ru_nvcsw, i_stats->rusage[0].ru_nivcsw);
		write(1, output_buffer, strlen(output_buffer));
	}
*/

/* cpu cycles/pingpong for pong
	if (config.set_affinity == true) {

		snprintf(output_buffer, output_buffer_len, ", pong: %d.%d cyc.",
			(int)i_stats->cpi[1], (int)(i_stats->cpi[1] * 100.0L) & 100);
		write(1, output_buffer, strlen(output_buffer));
		snprintf(output_buffer, output_buffer_len, ", %ld csw",
			i_stats->csw[1]);
		write(1, output_buffer, strlen(output_buffer));
	}
*/

	write(1, "\n", 1);
}

void store_last_stats(struct interval_stats_struct *i_stats) {
	/* cleanup things for the next time we come back */
	memcpy((void *)&run_data->thread_stats[0].last_rusage,
		(void *)&run_data->thread_stats[0].rusage, sizeof(struct rusage));
	memcpy((void *)&run_data->thread_stats[1].last_rusage,
		(const void *)&run_data->thread_stats[1].rusage, sizeof(struct rusage));

	run_data->last_ping_count = i_stats->current_count;
	run_data->last_stats_time = i_stats->current_time;
	run_data->thread_stats[0].last_tsc = run_data->thread_stats[0].tsc;
	run_data->thread_stats[1].last_tsc = run_data->thread_stats[1].tsc;

	if (i_stats->current_time >= run_data->timeout_time) {
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

int do_monitor_work(void) {
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
	mb();
	run_data->start = true; /* tell the child threads to begin */
	mb();
	config.comm_begin();

	run_data->start_time = run_data->last_stats_time = get_time();
	run_data->timeout_time = (double)run_data->start_time + (double)config.runtime;

	while (run_data->stop != true) {
		sigsuspend(&signal_mask);
	}
	stop_handler(1);

	return 0;
}

static int do_fork(void) {
	int thread_num = 0;
	int pid;

	pid = fork();
	if (pid == 0) { /* child proc */ /* ping */
		do_thread_work(thread_num);
	} else if (pid > 0) {
		run_data->thread_info[thread_num].pid = pid;

		thread_num ++;

		pid = fork();
		if (pid == 0) { /* child proc */ /* pong */
			do_thread_work(thread_num);
			exit(0);
		} else if (pid > 0) {
			run_data->thread_info[thread_num].pid = pid;
		} else {
			printf("Error while trying to fork: %d: %s\n",
				run_data->thread_info[thread_num].pid, strerror(errno));
			exit(-1);
		}
	} else {
		printf("Error while trying to fork: %d: %s\n",
			run_data->thread_info[thread_num].pid, strerror(errno));
		exit(-1);
	}

	return 0;
}
static int do_clone(void) {
	int thread_num = 0;
	int clone_flags;

	run_data->thread_info[0].stack = calloc(1, STACK_SIZE); /* ping thread */
	run_data->thread_info[1].stack = calloc(1, STACK_SIZE); /* pong thread */

	if ((run_data->thread_info[0].stack == 0) || (run_data->thread_info[1].stack == 0)) {
		perror("calloc: could not allocate stack");
		exit(1);
	}

//	clone_flags = CLONE_FS | CLONE_FILES | CLONE_PARENT | CLONE_VM | SIGCHLD;
	clone_flags = CLONE_FS | CLONE_FILES | SIGCHLD;

	run_data->thread_info[thread_num].tid = clone(&thread_function,
		(char *) run_data->thread_info[thread_num].stack + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num],
		&run_data->thread_info[thread_num].ptid, NULL, &run_data->thread_info[thread_num].ctid
		);

	thread_num++;

	run_data->thread_info[thread_num].tid = clone(&thread_function,
		(char *) run_data->thread_info[thread_num].stack + STACK_SIZE, clone_flags,
		(void *)&run_data->thread_info[thread_num],
		&run_data->thread_info[thread_num].ptid, NULL, &run_data->thread_info[thread_num].ctid
		);
	/* what about CLONE_SIGHAND and CLONE_VM ? */


	if ((run_data->thread_info[0].pid == -1) || (run_data->thread_info[1].pid == -1)) {
		perror("clone");
		exit(2);
	}

	return 0;
}

static int do_pthread(void) {
	int thread_num = 0;
	pthread_attr_t attr;
	int ret;

	if ((ret = pthread_attr_init(&attr)) != 0) {
		errno = ret; perror("pthread_attr_init"); exit(-1);
	}
	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		errno = ret ; perror("pthread_attr_setdetachstate"); exit(-1);
	}

	if ((ret = pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num])) != 0) {
		errno = ret;
		printf("Error occurred while starting 'ping' thread: %s\n", strerror(errno));
		exit(-1);
	}

	thread_num ++;

	pthread_create((pthread_t *)&run_data->thread_info[thread_num].thread_id, &attr,
		&pthread_function, (void *)&run_data->thread_info[thread_num]);


	if ((ret = pthread_attr_destroy(&attr)) != 0) {
		errno = ret; perror("pthread_attr_destroy"); exit(-1);
	}

	return 0;
}

static void init_threads(void) {

	memset(run_data, 0, sizeof(struct run_data_struct));

	run_data->stop = false;
	run_data->rusage_req_in_progress = false;
	run_data->rusage_req[0] = run_data->rusage_req[1] = false;
/*
	memset(&run_data->thread_info[0], 0, sizeof(struct thread_info_struct));
	memset(&run_data->thread_info[1], 0, sizeof(struct thread_info_struct));
	memset((void *)&run_data->thread_stats[0], 0, sizeof(struct thread_stats_struct));
	memset((void *)&run_data->thread_stats[1], 0, sizeof(struct thread_stats_struct));
*/
	run_data->thread_info[0].thread_num = 0;
	run_data->thread_info[1].thread_num = 1;
	strncpy(run_data->thread_info[0].thread_name, "ping_thread", 12);
	strncpy(run_data->thread_info[1].thread_name, "pong_thread", 12);

	run_data->thread_stats[0].sched_data = map_shared_area(PROC_PID_SCHED_SIZE, config.size_align_flag);
	run_data->thread_stats[1].sched_data = map_shared_area(PROC_PID_SCHED_SIZE, config.size_align_flag);

}

int start_threads(void) {
	printf("parent process is %d\n", getpid());

	init_threads();

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

	do_monitor_work();

	return 0;
}
