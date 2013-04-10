#include "sem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>

sem_t *sems[2];
const char *sem_names[] = { "ping", "pong" };

int make_sem_pair(int fd[2]) {
	static int sem_num = 0;
	sem_t *ret;
	/* need an _sems and _names */


	if ((ret = sem_open(sem_names[sem_num], O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		printf("Error opening semaphore '%s': %s\n", sem_names[sem_num], strerror(errno));
		exit(-1);
	}

	fd[0] = fd[1] = sem_num;
	sems[sem_num] = ret;

//	sems[1] = sem_open(config.sem_names[1], O_CREAT, S_IRUSR | S_IWUSR, 0);

	sem_num ++;
	return 0;
}

inline int do_send_sem(int fd) {
        return ! sem_post(sems[fd]);
}
inline int do_recv_sem(int fd) {
        int err;

        if (config.comm_mode == comm_mode_sem) {
                err = sem_wait(sems[fd]);
        } else { /* comm_mode_busy_sem */
                do {
                        if ((err = sem_trywait(sems[fd])) == -1)
                                err = errno;
                } while (err == EAGAIN);
        }
        return ! err;
}
int cleanup_sem() {
	sem_close(sems[0]);
	sem_unlink(sem_names[0]);
	sem_close(sems[1]);
	sem_unlink(sem_names[1]);

	return 0;
}
