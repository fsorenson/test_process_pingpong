#include "inotify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <errno.h>



#define EVENT_SIZE	(sizeof(struct inotify_event))
#define EVENT_BUF_SIZE	(1024 * (EVENT_SIZE + 16))

struct ino_info_struct {
	int ino_fd;
	int file_fd;
	int wd;
	char event_buf[EVENT_BUF_SIZE];
};
static struct ino_info_struct *ino_info;


const char *ino_names[] = { "/dev/shm/ping.ino", "/dev/shm/pong.ino" };


int make_inotify_pair(int fd[2]) {
	static int ino_num = 0;

	if (ino_num == 0) {
		ino_info = mmap(NULL, sizeof(struct ino_info_struct) * 2,
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	}

	ino_info[ino_num].ino_fd = inotify_init();
	if (ino_info[ino_num].ino_fd < 0) {
		perror("inotify_init");
		exit(1);
	}

	ino_info[ino_num].file_fd = open(ino_names[ino_num],
		O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (ino_info[ino_num].file_fd < 0) {
		printf("Error opening '%s'.  Error was '%m'\n", ino_names[ino_num]);
		exit(1);
	}

	ino_info[ino_num].wd = inotify_add_watch(ino_info[ino_num].ino_fd,
		ino_names[ino_num], IN_ALL_EVENTS);

	fd[0] = fd[1] = ino_num;

	ino_num ++;
	return 0;
}

inline int __PINGPONG_FN do_ping_inotify(int thread_num) {
	struct inotify_event *ino_event;
	long long counter;
	long long length;

	while (1) {
		run_data->ping_count ++;

		counter = 0;
		ftruncate(ino_info[thread_num].file_fd, 0);

		/* blocking read */
		length = read(ino_info[thread_num ^ 1].ino_fd, ino_info[thread_num].event_buf, EVENT_BUF_SIZE);
		if (length < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			printf("Error reading from inotify fd in thread %d (error was '%m')\n", thread_num);
		}

		while (counter < length) {
			ino_event = (struct inotify_event *) &ino_info[thread_num].event_buf[counter];
			counter += (long long)EVENT_SIZE + (long long)ino_event->len;
		}
	}
}

inline int __PINGPONG_FN do_pong_inotify(int thread_num) {
	struct inotify_event *ino_event;
	long long counter;
	long long length;

	while (1) {
		counter = 0;

		/* blocking read */
		length = read(ino_info[thread_num ^ 1].ino_fd, ino_info[thread_num].event_buf, EVENT_BUF_SIZE);
		if (length < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}

			printf("Error reading from inotify fd in thread %d (error was '%m')\n", thread_num);
		}

		while (counter < length) {
			ino_event = (struct inotify_event *) &ino_info[thread_num].event_buf[counter];
			counter += (long long)EVENT_SIZE + (long long)ino_event->len;
		}

		ftruncate(ino_info[thread_num].file_fd, 0);
	}
}


int cleanup_inotify() {
	munmap(ino_info, sizeof(struct ino_info_struct) * 2);

	return 0;
}

void __attribute__((constructor)) comm_add_inotify() {
        struct comm_mode_init_info_struct init_info;
	struct comm_mode_ops_struct ops;

	memset(&init_info, 0, sizeof(struct comm_mode_init_info_struct));
	init_info.name = strdup("inotify");
	init_info.help_text = strdup("use inotify to watch for file modifications");

	memset(&ops, 0, sizeof(struct comm_mode_ops_struct));
	ops.comm_make_pair = make_inotify_pair;
	ops.comm_do_ping = do_ping_inotify;
	ops.comm_do_pong = do_pong_inotify;
	ops.comm_cleanup = cleanup_inotify;


	comm_mode_do_initialization(&init_info, &ops);
}

ADD_COMM_MODE(inotify, comm_add_inotify);

