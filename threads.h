


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_process_pingpong.h"
#include "threads.h"
#include "setup.h"


#include "units.h"
#include "sched.h"


/* thread startup, execution, handlers, etc */


void show_stats(int signum) {
	(void)signum;

	static char buffer[256];

	static unsigned long long last_count = 0;
	unsigned long long current_count = run_data->ping_count;
	unsigned long long interval_count = current_count - last_count;

	static long double last_stats_time = 0;
	long double current_stats_time = get_time();
	long double interval_time;
	struct rusage interval_rusage[2];
	integer_fixed_point_t run_time_int;

	run_data->rusage_req_in_progress = true;
	run_data->rusage_req[0] = true;
	run_data->rusage_req[1] = true;

	if (last_stats_time == 0)
		last_stats_time = run_data->start_time;

	interval_time = current_stats_time - last_stats_time;

	run_time_int = f_to_fp(1000, current_stats_time - run_data->start_time);


	snprintf(buffer, 255, "%2ld.%3ld - %llu iterations -> %s\n",
		run_time_int.i, run_time_int.dec,
		interval_count, subsec_string(NULL, interval_time / interval_count, 3));
	buffer[255] = 0;
	write(1, buffer, strlen(buffer));
	if (current_stats_time >= run_data->timeout_time)
		run_data->stop = true;

	while ((run_data->rusage_req_in_progress == true)) {
		if ((run_data->rusage_req[0] == false) && (run_data->rusage_req[1] == false))
			run_data->rusage_req_in_progress = false;
		if (run_data->rusage_req_in_progress == true)
			__sync_synchronize();
	}

	interval_rusage[0].ru_nvcsw = run_data->rusage[0].ru_nvcsw - run_data->last_rusage[0].ru_nvcsw;
	interval_rusage[0].ru_nivcsw = run_data->rusage[0].ru_nivcsw - run_data->last_rusage[0].ru_nivcsw;
	interval_rusage[1].ru_nvcsw = run_data->rusage[1].ru_nvcsw - run_data->last_rusage[1].ru_nvcsw;
	interval_rusage[1].ru_nivcsw = run_data->rusage[1].ru_nivcsw - run_data->last_rusage[1].ru_nivcsw;

	memcpy((void *)run_data->last_rusage, (void *)run_data->rusage, sizeof(struct rusage) * 2);

	snprintf(buffer, 255, "\tping csw: vol=%ld, invol=%ld; ping csw: vol=%ld, invol=%ld\n",
		interval_rusage[0].ru_nvcsw, interval_rusage[0].ru_nivcsw,
		interval_rusage[1].ru_nvcsw, interval_rusage[1].ru_nivcsw);


//	snprintf(buffer, 255, "context switches: vol=%ld, invol=%ld\n", usage.ru_nvcsw, usage.ru_nivcsw);
	buffer[255] = 0;
	write(1, buffer, strlen(buffer));

	last_count = current_count;
	last_stats_time = current_stats_time;
}

void child_handler(int signum) {
	(void)signum;
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
		if (pid == run_data->ping_pid) { /* ping died */
//printf("ping process died\n");
			run_data->ping_pid = -1;
			run_data->stop = true;

//			if (run_data->pong_pid != -1)
//				kill(run_data->pong_pid, SIGINT);
			stop_timer();
		} else if (pid == run_data->pong_pid) {
//printf("pong process died\n");
			run_data->pong_pid = -1;
			run_data->stop = true;

//			if (run_data->ping_pid != -1)
//				kill(run_data->ping_pid, SIGINT);
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
	run_data->start_time = get_time();
	run_data->timeout_time = run_data->start_time + config.max_execution_time * 1.0;

	while ((run_data->stop != true) || (run_data->ping_pid != -1) || (run_data->pong_pid != -1)) {
		sigsuspend(&signal_mask);
	}

	config.cleanup();
	return 0;
}

/* signal to receive if my parent process dies */
void on_parent_death(int signum) {
	int ret;

	if ((ret = prctl(PR_SET_PDEATHSIG, signum)) == -1) {
		perror("Failed to create message to be delivered upon my becoming an orphan: %m\n");
	}
}

void do_ping_work() {
	rename_thread("ping-thread");

	on_parent_death(SIGINT);
	setup_crash_handler();

	if (config.cpu[0] != -1) {
		set_affinity(config.cpu[0]);
	} else { /* affinity not set */
//		printf ("Affinity not set, however ping *currently* running on cpu %d\n", sched_getcpu());
	}

	while (run_data->stop != true) {
		run_data->ping_count ++;

		while ((config.do_send(config.mouth[0]) != 1) && (run_data->stop != true));
		while ((config.do_recv(config.ear[0]) != 1) && (run_data->stop != true));

		if (run_data->rusage_req[0] == true) {
			getrusage(RUSAGE_SELF, (struct rusage *)&run_data->rusage[0]);
			run_data->rusage_req[0] = false;
		}
	}

	/* cleanup */
	config.cleanup();
	exit(0);
}
int ping_thread_function(void *argument) {
	(void)argument;

	do_ping_work();
	return 0;
}
void *ping_pthread_function(void *argument) {
	(void)argument;

	do_ping_work();
	return NULL;
}

void do_pong_work() {
	rename_thread("pong-thread");

	on_parent_death(SIGINT);
	setup_crash_handler();

	if (config.cpu[1] != -1) {
		set_affinity(config.cpu[1]);
	} else {
//		printf("Affinity not set, however pong *currently* running on cpu %d\n", sched_getcpu());
	}

	while (run_data->stop != true) {
		while ((config.do_recv(config.ear[1]) != 1) && (run_data->stop != true));
		while ((config.do_send(config.mouth[1]) != 1) && (run_data->stop != true));

		if (run_data->rusage_req[1] == true) {
			getrusage(RUSAGE_SELF, (struct rusage *)&run_data->rusage[1]);
			run_data->rusage_req[1] = false;
		}
	}
	config.cleanup();
	exit(0);
}
int pong_thread_function(void *argument) {
	(void)argument;

	do_pong_work();
	return 0;
}
void *pong_pthread_function(void *argument) {
	(void)argument;

	do_pong_work();
	return NULL;
}

int do_fork() {
	pid_t pid;

	pid = fork();
	if (pid == 0) { /* child proc */
		do_pong_work();
	} else if (pid > 0) {
		run_data->pong_pid = pid;

		pid = fork();
		if (pid == 0) { /* child proc */
			do_ping_work();
		} else if (pid > 0) {
			run_data->ping_pid = pid;

			/* setup child handlers */
			do_monitor_work();

			if (run_data->ping_pid > 0)
				kill(run_data->ping_pid, SIGINT);
			if (run_data->pong_pid > 0)
				kill(run_data->pong_pid, SIGINT);
		} else {
			printf("Error while trying to fork: %d: %m\n", run_data->ping_pid);
			exit(-1);
		}
	} else {
		printf("Error while trying to fork: %d: %m\n", run_data->pong_pid);
		exit(-1);
	}

	return 0;
}
int do_clone() {

	config.stack[0] = calloc(1, STACK_SIZE); /* ping thread */
	config.stack[1] = calloc(1, STACK_SIZE); /* pong thread */

	if ((config.stack[0] == 0) || (config.stack[1] == 0)) {
		perror("calloc: could not allocate stack");
		exit(1);
	}

	run_data->ping_pid = clone(&ping_thread_function,
		(char *) config.stack[0] + STACK_SIZE,
		CLONE_FS | CLONE_FILES | SIGCHLD,
		NULL);

	run_data->pong_pid = clone(&pong_thread_function,
		(char *) config.stack[1] + STACK_SIZE,
		CLONE_FS | CLONE_FILES | SIGCHLD,
		NULL);
	/* what about CLONE_SIGHAND and CLONE_VM ? */

	if ((run_data->ping_pid == -1) || (run_data->pong_pid == -1)) {
		perror("clone");
		exit(2);
	}
	do_monitor_work();
//	free(config.stack[1]);

	return 0;
}

int do_pthread() {
	pthread_attr_t attr;
	int ret;

	if ((ret = pthread_attr_init(&attr)) != 0) {
		errno = ret; perror("pthread_attr_init"); exit(-1);
	}
	if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		errno = ret ; perror("pthread_attr_setdetachstate"); exit(-1);
	}

	pthread_create((pthread_t *)&run_data->ping_thread, &attr, &ping_pthread_function, NULL);
	pthread_create((pthread_t *)&run_data->pong_thread, &attr, &pong_pthread_function, NULL);

	if ((ret = pthread_attr_destroy(&attr)) != 0) {
		errno = ret; perror("pthread_attr_destroy"); exit(-1);
	}

	do_monitor_work();

	return 0;
}

int start_threads() {
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

