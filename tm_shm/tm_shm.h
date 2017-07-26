#ifndef __TM_SHM_H__
#define __TM_SHM_H__

#include <sys/types.h>

#define FILE_PATH_LEN 4096

struct tm_shm_mgr_t {
	/*! 0: 表示无效 */
	key_t	key;
	int		shmid;
	size_t	size;
	void	*shm;
};


//-----------------------------------------------------------
// helper functions
//-----------------------------------------------------------

/* @brief 请确保传入该函数的 key 在全系统范围是不重复的 */
int do_tm_shm_create_init(key_t key, size_t size, void *init_buf, size_t init_buf_len);




//-----------------------------------------------------------
// exported interface functions
//-----------------------------------------------------------
/*
 * @brief 创建一个初始化成 0 的 shm segment
 * 注意: 不允许创建 IPC_PRIVATE 类型的 shm (key不能是0);
 */
int tm_shm_create(key_t key, size_t size);


/*
 * @brief 注意:
 * 如果 init_buf_len > size,
 * 		则仅把 init_buf 的前 init_buf_len字节 初始化到新创建的 shm 中;
 * 注意: 不允许创建 IPC_PRIVATE 类型的 shm (key不能是0);
 */
int tm_shm_create_init(key_t key, size_t size, void *init_buf, size_t init_buf_len);

/*
 * @brief 创建一个初始化成 0 的 shm segment
 * 若在system-wide内有相同key的shm, 则返回错误
 */
int tm_shm_create_new(key_t key, size_t size);

/*
 * @brief 注意:
 * 1. 如果 init_buf_len > size,
 * 		则仅把 init_buf 的前 init_buf_len字节 初始化到新创建的 shm 中;
 *
 * 2. 如果 key == IPC_PRIVATE (0); 则是创建私有shm
 * 		(其它进程无法attach, 并只能通过shmid来删除)
 *
 * 3. 若在system-wide内有相同key的shm, 则返回错误
 */
int tm_shm_create_new_init(key_t key, size_t size, void *init_buf, size_t init_buf_len);

/**
 * @brief attach 到 new_shmid 的 shm 上, 同时 detach old_shmid 的 shm;
 * 注意: 第一次 update_attach 的时候, 要给 shm_mgr->key 赋值;
 */
int tm_shm_update_attach(struct tm_shm_mgr_t *shm_mgr);

/**
 * @brief 测试 system-wide 是否有 key 的 shm
 * @return 0: key 的 shm 还没建立; 1: key 的 shm 已经建立; -1: 出错
 */
int tm_shm_test_shm_exist(key_t key);

/*added by singku for mmap xml*/
typedef int (*call_back_func_t)(void* addr, const char* path);

typedef struct cfg_info{
    size_t  mem_size;   //此配置文件对应要申请的内存大小(实际大小是2倍的mem_size，另一半用于更新)
    size_t  meta_size;  //内存中每个对象(元数据)的大小
    char    *cfg_mmap_file_path;  //配置文件对应的mmap文件的路径(spirit, weapon, item...)
    char    *cfg_file_path;  //配置文件的路径
    call_back_func_t call_back; //加载配置文件的回调接口,由用户决定如何使用这块内存.必须返回0表示成功-1表示失败
    char    *addr;      //mmap申请到的内存地址
} cfg_info_t;

//==added for init on mmap
/**
 * @brief 在mmap上加载配置表
 * @param c_info: 配置文件信息。
 * @return 0  成功 -1失败.
 */
int init_cfg_on_mmap(cfg_info_t *c_info);

/**
 * @brief 在mmap上释放配置表.
 */
void destroy_cfg_from_mmap(cfg_info_t* c_info);

/**
 * @brief 在mmap上更新配置表
 */
int update_cfg_on_mmap(cfg_info_t *c_info);

int attach_cfg_to_mmap(cfg_info_t *c_info);
void detach_cfg_from_mmap(cfg_info_t *c_info);

/**/
#endif /* __TM_SHM_H__ */
