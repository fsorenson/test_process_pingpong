#include "benaphore.h"
#include "comms.h"
#include "test_process_pingpong.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>

sem_t *benaphore_sem;
volatile long volatile *benaphore_atom;
volatile long volatile *benaphore_value;
const char *ben_name = "benaphore";

int make_ben_pair(int fd[2]) {
	static int ben_num = 0;
	sem_t *ret;

	if (ben_num == 0) {
		if ((ret = sem_open(ben_name, O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
			printf("Error opening semaphore '%s': %s\n", ben_name, strerror(errno));
			exit(-1);
		}
		benaphore_atom = mmap(NULL, sizeof(long), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		benaphore_sem = ret;
	}

	fd[0] = fd[1] = ben_num;

//	sems[1] = sem_open(config.sem_names[1], O_CREAT, S_IRUSR | S_IWUSR, 0);

	ben_num ++;
	return 0;
}

void acquire_ben(void) {
	long previous_value;
	previous_value = atomic_add(&benaphore_atom, 1);

	if(previous_value > 0)
		acquire_sem(benaphore_sem);
}
void release_ben(void) {
	long previous_value;
	previous_value = atomic_add(&benaphore_atom, -1);

	if (previous_value > 1)
		release_sem(benaphore_sem);
}


inline int do_send_ben(int fd) {
	int ret;

	acquire_ben();

	
//        return ! sem_post(bens[fd]);

	release_ben();

	return 1;
}
inline int do_recv_ben(int fd) {
        int err;

	err = ben_wait(bens[fd]);
        return ! err;
}
int cleanup_ben(void) {
	sem_close(bens[0]);
	sem_unlink(ben_names[0]);
	sem_close(bens[1]);
	sem_unlink(ben_names[1]);

	return 0;
}


static struct comm_mode_ops_struct comm_ops_ben = {
	.comm_make_pair         = make_ben_pair,
	.comm_do_ping           = do_ping_ben,
	.comm_do_pong           = do_pong_ben,
	.comm_cleanup           = cleanup_ben
};



ADD_COMM_MODE(ben, "benaphore", &comm_ops_ben);
