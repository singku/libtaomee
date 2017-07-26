#include <errno.h>

#include <sys/ipc.h>
#include <sys/sem.h>
//#include <signal.h>
//#include <stdio.h>
#include <unistd.h>
//#include <stdlib.h>
//#include <string.h>
#include <sys/types.h>

#include "lock.h"

int sem_init(key_t key)
{
	int ret_semid = semget (key, 1, IPC_CREAT | 0664);
	if (ret_semid != -1) {
		union semun arg;
		arg.val = 1;
		semctl (ret_semid, 0, SETVAL, arg);
	}

	return ret_semid;
}

int sem_unlock(int semid)
{
	struct sembuf op;
	op.sem_num  = 0;
	op.sem_flg  = SEM_UNDO;
	op.sem_op   = 1;
	while (semop(semid, &op, 1) == -1) {
		if (errno != EINTR) {
			return -1;
		}
	}

	return 0;
}

int sem_lock(int semid)
{
	struct sembuf op;

	op.sem_num  = 0;
	op.sem_flg  = SEM_UNDO;
	op.sem_op   = -1;

	while (semop(semid, &op, 1) == -1) {
		if (errno != EINTR) {
			return -1;
		}
	}

	return 0;
}
