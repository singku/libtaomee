/**
 *============================================================
 *  @file      shmq.h
 *  @brief     共享内存队列。目前的shm_cq只适用于一个进程只读，另外一个进程只写的应用。\n
 *             具体例子请参见/samples/shmq/
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef TAOMEE_SHMQ_H_
#define TAOMEE_SHMQ_H_

#include <stdint.h>

#include <libtaomee/atomic.h>

/**
 * @brief 共享内存队列常量定义
 */
enum shmq_const {
	/*! 64：共享内存名字最大长度，包括0字符 */
	shmq_name_sz	= 64
};

/**
  * @struct shm_cq
  * @brief 环形队列（Circular Queue）头节点
  */
struct shm_cq {
	/*! 队列头偏移量 */
	volatile uint32_t	head;
	/*! 队列尾偏移量 */
	volatile uint32_t	tail;
	/*! 为队列分配的共享内存总大小（byte）*/
	uint32_t			shm_size;
	/*! 队列中每个元素的最大大小（byte）*/
	uint32_t			elem_max_sz;
	/*! 共享内存名字 */
	char				name[shmq_name_sz];
	// TODO: lock
};

/**
 * @typedef shm_cq_t
 * @brief typedef of shm_cq
 */
typedef struct shm_cq shm_cq_t;

// #pragma pack() // shm_cq need not packed

/**
 * @brief 分配一片共享内存，并以此为基础创建一条队列
 * @param name 共享内存名字，长度不能超过63个字节，否则不能创建队列
 * @param cq_size 队列大小，必须比data_max_sz大100个字节，且小于2000000000字节
 * @param data_max_sz 插入队列的数据最大不能超过data_max_sz个字节
 * @return 成功则返回共享内存队列的指针，失败则返回0
 * @note 非线程安全，也非进程安全
 * @see shm_cq_destroy, shm_cq_attach
 */
shm_cq_t* shm_cq_create(const char* name, uint32_t cq_size, uint32_t data_max_sz);
/**
 * @brief 销毁共享内存队列
 * @param cq 需要销毁的队列
 * @return 0成功，-1失败
 * @see shm_cq_create
 */
int shm_cq_destroy(shm_cq_t* cq);
/**
 * @brief 依附到一条已经创建好了的共享内存队列
 * @param name 想要依附到的共享内存名字
 * @return 成功则返回共享内存队列的指针，失败则返回0
 * @see shm_cq_create, shm_cq_detach
 */
shm_cq_t* shm_cq_attach(const char* name);
/**
 * @brief 取消对队列cq的依附关系。shm_cq_destroy后，还要等所有其它进程都shm_cq_detach后，cq指向的内存才真正被释放。
 * @param cq 共享内存队列
 * @return 成功返回0，失败返回-1
 * @see shm_cq_attach, shm_cq_destroy
 */
int shm_cq_detach(shm_cq_t* cq);
/**
 * @brief 从队列头中返回数据给data，并且移除该队头
 * @param q 共享内存队列
 * @param data 出列成功后，*data中得到指向该队头的指针
 * @return 成功则返回得到的数据的长度；如果队列为空，则返回0
 * @see shm_cq_push
 */
uint32_t shm_cq_pop(shm_cq_t* q, void** data);
/**
 * @brief 往队尾插入长度为len的数据data
 * @param q 共享内存队列
 * @param data 插入到队尾的数据 
 * @param len data的长度
 * @return 成功返回0，失败（队列满了）返回-1
 * @see shm_cq_pop
 */
int shm_cq_push(shm_cq_t* q, const void* data, uint32_t len);

#endif // TAOMEE_SHMQ_H_

