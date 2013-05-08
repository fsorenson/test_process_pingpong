#include "mq.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

static char *mqueue_path[2];
mqd_t mqueue[2];
char mq_msg[2];
unsigned int mq_prio[2];
//static timespec mq_timeout;

int make_mq_pair(int fd[2]) {
	static int mq_num = 0;
	static long int max_mq_prio = -1;
	mqd_t ret;
	struct mq_attr attr;
	int flags = O_RDWR | O_CREAT;
	int perms = DEFFILEMODE;

#ifdef _SC_MQ_PRIO_MAX
	if (max_mq_prio == -1) {
		max_mq_prio = sysconf(_SC_MQ_PRIO_MAX) - 1;
	}
#else
	max_mq_prio = MQ_PRIO_MAX;
#endif

	attr.mq_flags = 0;
	attr.mq_maxmsg = 1;
	attr.mq_msgsize = 1;
	attr.mq_curmsgs = 0;

	if (mq_num == 0) {
		asprintf(&mqueue_path[mq_num], "/%sping", basename(config.argv0));
	} else {
		asprintf(&mqueue_path[mq_num], "/%spong", basename(config.argv0));
	}

	if ((ret = mq_open(mqueue_path[mq_num], flags, perms, &attr)) == -1) {
		printf("Could not create mq '%s': %s\n", mqueue_path[mq_num], strerror(errno));
		exit(-1);
	}
	mqueue[mq_num] = ret;

//	mq_getattr(mqueue[mq_num], &attr);

	mq_prio[mq_num] = (unsigned int) max_mq_prio;
	fd[0] = mq_num;
	fd[1] = mq_num;
	mq_msg[mq_num] = '1';

	mq_num ++;

	return 0;
}

inline int do_send_mq(int fd) {

	return ! mq_send(mqueue[fd], &mq_msg[fd], 1, mq_prio[fd]);
}
inline int do_recv_mq(int fd) {

	return (int)(mq_receive(mqueue[fd], &mq_msg[fd], 1, &mq_prio[fd]));
}

int cleanup_mq(void) {
	mq_close(mqueue[0]);
	mq_close(mqueue[1]);

	mq_unlink(mqueue_path[0]);
	mq_unlink(mqueue_path[1]);
	free(mqueue_path[0]);
	free(mqueue_path[1]);

	return 0;
}

static struct comm_mode_ops_struct comm_ops_mq = {
	.comm_make_pair = make_mq_pair,
	.comm_do_send = do_send_mq,
	.comm_do_recv = do_recv_mq,
	.comm_cleanup = cleanup_mq
};

ADD_COMM_MODE(mq, "", &comm_ops_mq);
