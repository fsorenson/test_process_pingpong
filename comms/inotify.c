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
		printf("Error opening '%s'.  Error was '%s'\n", ino_names[ino_num], strerror(errno));
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
			printf("Error reading from inotify fd in thread %d (error was '%s')\n",
				thread_num, strerror(errno));
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

			printf("Error reading from inotify fd in thread %d (error was '%s')\n",
				thread_num, strerror(errno));
		}

		while (counter < length) {
			ino_event = (struct inotify_event *) &ino_info[thread_num].event_buf[counter];
			counter += (long long)EVENT_SIZE + (long long)ino_event->len;
		}

		ftruncate(ino_info[thread_num].file_fd, 0);
	}
}


int cleanup_inotify(void) {
	munmap(ino_info, sizeof(struct ino_info_struct) * 2);

	return 0;
}

static struct comm_mode_ops_struct comm_ops_inotify = {
	.comm_make_pair = make_inotify_pair,
	.comm_do_ping = do_ping_inotify,
	.comm_do_pong = do_pong_inotify,
	.comm_cleanup = cleanup_inotify
};

ADD_COMM_MODE(inotify, "use inotify to watch for file modifications", &comm_ops_inotify);
