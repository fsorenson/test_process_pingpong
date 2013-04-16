#include "inotify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/inotify.h>



#define EVENT_SIZE	(sizeof(struct inotify_event))
#define EVENT_BUF_SIZE	(1024 * (EVENT_SIZE + 16))
static int ino_fd[2];
static int open_fd[2];
int ino_watch[2];
char ino_buf[2][EVENT_BUF_SIZE];

const char *ino_names[] = { "/dev/shm/ping.ino", "/dev/shm/pong.ino" };


int make_inotify_pair(int fd[2]) {
	static int ino_num = 0;

	ino_fd[ino_num] = inotify_init();
	if (ino_fd[ino_num] < 0) {
		perror("inotify_init");
		exit(1);
	}

	open_fd[ino_num] = open(ino_names[ino_num],
		O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	ino_watch[ino_num] = inotify_add_watch(ino_fd[ino_num],
		ino_names[ino_num], IN_ACCESS | IN_ATTRIB | IN_MODIFY);

	fd[0] = fd[1] = ino_num;

	ino_num ++;
	return 0;
}

inline int __PINGPONG_FN do_ping_inotify(int thread_num) {
	int counter;
	int length;

	while (1) {
		run_data->ping_count ++;

		counter = 0;
		write(open_fd[thread_num], "X", 1);
		length = read(ino_fd[thread_num ^ 1], ino_buf[thread_num], EVENT_BUF_SIZE);
		if (length < 0) {
			printf("Error reading from inotify fd in thread %d (error was '%m')\n", thread_num);
		{
	}
}

inline int __PINGPONG_FN do_pong_inotify(int thread_num) {
	int counter;
	int length;

	while (1) {
		counter = 0;
		length = read(ino_fd[thread_num ^ 1], ino_buf[thread_num], EVENT_BUF_SIZE);
		if (length < 0) {
			printf("Error reading from inotify fd in thread %d (error was '%m')\n", thread_num);
		}
		write(open_fd[thread_num], "X", 1);
	}
}


int cleanup_inotify() {

	return 0;
}

void __attribute__((constructor)) comm_add_inotify() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = "inotify";
	init_info.help_text = "use inotify to watch for file modifications";

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_inotify_pair;
	ops.comm_do_ping = do_ping_inotify;
	ops.comm_do_pong = do_pong_inotify;
	ops.comm_cleanup = cleanup_inotify;


	comm_mode_do_initialization(&init_info, &ops);
}

ADD_COMM_MODE(inotify, comm_add_inotify);

