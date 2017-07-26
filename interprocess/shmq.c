#include <assert.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>     /* For O_* constants */
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "shmq.h"

enum {
	/* share memory circular queue max size */
	shm_cq_max_sz = 2000000000,
};

#pragma pack(1)

struct shm_block {
	uint32_t	len;
	uint8_t		data[];
};

typedef struct shm_block shm_block_t;

#pragma pack()

//------------------------------------------------------------------------
inline shm_block_t* shm_cq_head(shm_cq_t* q);
inline shm_block_t* shm_cq_tail(shm_cq_t* q);
inline void shm_cq_align_head(shm_cq_t* q);
static int  shm_cq_align_tail(shm_cq_t* q, uint32_t len);
inline int  shm_cq_tail_align_wait(shm_cq_t* q);
inline int  shm_cq_push_wait(shm_cq_t* q, uint32_t len);
//------------------------------------------------------------------------

// TODO: check if the queue of 'name' is under usage
// TODO: process/thread saftey
shm_cq_t* shm_cq_create(const char* name, uint32_t cq_size, uint32_t data_max_sz)
{
	assert((cq_size < shm_cq_max_sz)
			&& (cq_size > data_max_sz)
			&& (cq_size >= (data_max_sz + sizeof(shm_block_t))));

	if (strlen(name) >= shmq_name_sz) {
		errno = ENAMETOOLONG;
		return 0;
	}

	int shmfd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (shmfd == -1) {
		return 0;
	}

	uint32_t size = cq_size + sizeof(shm_cq_t);
	int ret = ftruncate(shmfd, size);
	if (ret == -1) {
		close(shmfd);
		shm_unlink(name);
		return 0;
	}
	
	shm_cq_t* cq = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (cq == MAP_FAILED) {
		close(shmfd);
		shm_unlink(name);
		return 0;
	}

	close(shmfd);
	// init shared-memory circular queue
	cq->head  = sizeof(shm_cq_t);
	cq->tail  = sizeof(shm_cq_t);
	cq->shm_size    = size;
	cq->elem_max_sz = data_max_sz + sizeof(shm_block_t);
	strcpy(cq->name, name);

	return cq;
}

int shm_cq_destroy(shm_cq_t* cq)
{
	return -(shm_unlink(cq->name) || munmap(cq, cq->shm_size));
}

shm_cq_t* shm_cq_attach(const char* name)
{
	int shmfd = shm_open(name, O_RDWR, 0);
	if (shmfd == -1) {
		return 0;
	}

	// this call of mmap is used to get cq->shm_size only
	shm_cq_t* cq = mmap(0, sizeof(shm_cq_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (cq == MAP_FAILED) {
		close(shmfd);
		return 0;
	}

	// used to mmap shmfd again
	uint32_t sz = cq->shm_size;
	// unmap cq
	munmap(cq, sizeof(shm_cq_t));
	// mmap again with the real length of shm_cq
	cq = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);	
	if (cq == MAP_FAILED) {
		close(shmfd);
		return 0;
	}

	close(shmfd);
	return cq;
}

int shm_cq_detach(shm_cq_t* cq)
{
	return munmap(cq, cq->shm_size);
}

uint32_t shm_cq_pop(shm_cq_t* q, void** data)
{
	//queue is empty
	if (q->head == q->tail) {
		return 0;
	}

	shm_cq_align_head(q);

	//queue is empty
	if (q->head == q->tail) {
		return 0;
	}

	shm_block_t* cur_mb = shm_cq_head(q);

	assert(cur_mb->len <= q->elem_max_sz);
	
	*data = cur_mb->data;
	q->head += cur_mb->len;

	return cur_mb->len - sizeof(shm_block_t);
}

int shm_cq_push(shm_cq_t* q, const void* data, uint32_t len)
{
	assert((len > 0) && (len <= q->elem_max_sz - sizeof(shm_block_t)));

	uint32_t elem_len = len + sizeof(shm_block_t);
	if (shm_cq_align_tail(q, elem_len) == -1) {
		return -1;
	}

	shm_block_t* next_mb = shm_cq_tail(q);
	next_mb->len = elem_len;
	memcpy(next_mb->data, data, len);

	q->tail += elem_len;

	return 0;
}

//-------------------------------------------------------------------
inline shm_block_t* shm_cq_head(shm_cq_t* q)
{
	return (shm_block_t*)(((char*)q) + q->head);
}

inline shm_block_t* shm_cq_tail(shm_cq_t* q)
{
	return (shm_block_t*)(((char*)q) + q->tail);
}

inline void shm_cq_align_head(shm_cq_t* q)
{
	uint32_t head = q->head;
	if (head < q->tail) {
		return;
	}

	shm_block_t* pad = shm_cq_head(q);
	if (((q->shm_size - head) < sizeof(shm_block_t)) || (pad->len == 0xFFFFFFFF)) {
		q->head = sizeof(shm_cq_t);
	}
}

static int shm_cq_align_tail(shm_cq_t* q, uint32_t len)
{
	uint32_t tail_pos = q->tail;
	int surplus = q->shm_size - tail_pos;
	if (surplus >= len) {
		return shm_cq_push_wait(q, len);
	}

	if (shm_cq_tail_align_wait(q) == 0) {
		if (surplus >= sizeof(shm_block_t)) {
			shm_block_t* pad = shm_cq_tail(q);
			pad->len = 0xFFFFFFFF;
		}
		q->tail = sizeof(shm_cq_t);
		return shm_cq_push_wait(q, len);
	}

	return -1;
}

inline int shm_cq_tail_align_wait(shm_cq_t* q)
{
	int cnt;
	uint32_t tail = q->tail;
	for (cnt = 0; cnt != 10; ++cnt) {
		uint32_t head = q->head;
		if ((head > sizeof(shm_cq_t)) && (head <= tail)) {
			return 0;
		}
		usleep(5);
	}

	return -1;
}

inline int shm_cq_push_wait(shm_cq_t* q, uint32_t len)
{
	int cnt;
	uint32_t tail = q->tail;
	for (cnt = 0; cnt != 10; ++cnt) {
		uint32_t head = q->head;
		// q->elem_max_sz is added just to prevent overwriting the buffer that might be referred to currently
		if ((head <= tail) || (head >= (tail + len + q->elem_max_sz))) {
			return 0;
		}
		usleep(5);
	}
	
	return -1;
}

