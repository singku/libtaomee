#ifndef LIBTAOMEE_LOCK_H_
#define LIBTAOMEE_LOCK_H_

union semun {
	int val;                /* value for SETVAL */
	struct semid_ds* buf;   /* buffer for IPC_STAT, IPC_SET */
	unsigned short *array;  /* array for GETALL, SETALL */
	/* Linux specific part: */
	struct seminfo *__buf;  /* buffer for IPC_INFO */
};

int sem_init(key_t key);
int sem_unlock(int semid);
int sem_lock(int semid);

#endif // LIBTAOMEE_LOCK_H_

