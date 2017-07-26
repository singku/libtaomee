#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>

#include <glib.h>
#include <libgen.h>


#include <libtaomee/log.h>
#include <libtaomee/conf_parser/config.h>

#include <errno.h>

#include "tm_shm.h"


int init_cfg_on_mmap(cfg_info_t *c_info)
{
    if (c_info->meta_size > c_info->mem_size 
        || c_info->mem_size % c_info->meta_size != 0) {
        ERROR_RETURN(("init cfg on mmap failed: meta size"
                   " > mem_size or mem_size%meta_size!=0"), -1);
    }

    char path[FILE_PATH_LEN];
    snprintf(path, FILE_PATH_LEN, "%s", c_info->cfg_mmap_file_path);

    if (g_mkdir_with_parents(dirname(path), 0755) == -1) {
        ERROR_RETURN(("tm_dirty: failed to create dir: %s", path), -1);
    }

    int fd = open(c_info->cfg_mmap_file_path, O_CREAT| O_RDWR, 0644);

    if (fd == -1) {
        ERROR_RETURN(("open file %s failed:%s", c_info->cfg_mmap_file_path, strerror(errno)), -1);
    }
    
    char* pmap;

    ftruncate(fd, c_info->mem_size * 2);
    pmap = mmap(NULL, c_info->mem_size * 2, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (pmap == MAP_FAILED) {
        close(fd);
        ERROR_RETURN(("mmap failed:%s", strerror(errno)), -1); 
    }

    c_info->addr = pmap;
    if (c_info->call_back == NULL || c_info->call_back(c_info->addr, c_info->cfg_file_path) == -1) {
        destroy_cfg_from_mmap(c_info);
        ERROR_RETURN(("init failed: call back function is not"
                    " valid(fucn addr:%p) or call back return -1",
                    c_info->call_back), -1);
    }

    close(fd);
    return 0;
}

int update_cfg_on_mmap(cfg_info_t *c_info)
{
    if (c_info->meta_size > c_info->mem_size 
            || c_info->mem_size % c_info->meta_size != 0) {
        ERROR_RETURN(("update cfg on mmap failed: meta size"
                    " > mem_size or mem_size%meta_size!=0"), -1);
    }   
    char* pmap = c_info->addr + c_info->mem_size;

    if (c_info->call_back == NULL || c_info->call_back(pmap, c_info->cfg_file_path) == -1) {
        ERROR_RETURN(("update failed: call back function is not"
                    " valid(fucn addr:%p) or call back return -1",
                    c_info->call_back), -1);
    }
    
    //check meta info step by step
    char *p_use, *p_update;
    p_use = c_info->addr;
    p_update = pmap;
    int cmp_size = 0;

    while (cmp_size < (c_info->mem_size - c_info->meta_size)) {
        if(memcmp(p_use, p_update, c_info->meta_size) != 0) {
            memcpy(p_use, p_update, c_info->meta_size);
        }
        p_use += c_info->meta_size;
        p_update += c_info->meta_size;
        cmp_size += c_info->meta_size;
    }
    //check the last segment
    if (memcmp(p_use, p_update, (c_info->mem_size - c_info->meta_size)) != 0) {
        memcpy(p_use, p_update, c_info->mem_size - c_info->meta_size);
    }
    return 0;
}

void destroy_cfg_from_mmap(cfg_info_t* c_info)
{
    int ret = munmap(c_info->addr, c_info->mem_size * 2);
    if (ret == -1) {
        ERROR_LOG("mumap error:%s", strerror(errno));
    }
}

int attach_cfg_to_mmap(cfg_info_t* c_info)
{
    int fd = open(c_info->cfg_mmap_file_path, O_RDONLY, 0644);

    if (fd == -1) {
        ERROR_RETURN(("open file %s failed:%s", c_info->cfg_mmap_file_path, strerror(errno)), -1);
    }
    
    char* pmap;

    ftruncate(fd, c_info->mem_size * 2);
    pmap = mmap(NULL, c_info->mem_size * 2, PROT_READ, MAP_SHARED, fd, 0);
    if (pmap == MAP_FAILED) {
        close(fd);
        ERROR_RETURN(("mmap failed:%s", strerror(errno)), -1); 
    }

    c_info->addr = pmap;
    close(fd);
    return 0;
   
}

void detach_cfg_from_mmap(cfg_info_t* c_info)
{
    destroy_cfg_from_mmap(c_info);
}
//-----------------------------------------------------------
// helper functions
//-----------------------------------------------------------

/* @brief 请确保传入该函数的 key 在全系统范围是不重复的 */
int do_tm_shm_create_init(key_t key, size_t size, void *init_buf, size_t init_buf_len)
{
	
	int shmid = -1;
	void *shm = 0;
	size_t init_size = 0;

	shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600);
	/* 0600: S_IRUSR | S_IWUSR: 可读可写 */
	if (shmid == -1) {
		perror("error - do create shm failed: ");
		return -1;
	}

	shm = shmat(shmid, 0, 0);
	if (shm == (void *)-1) {
		perror("error - attach shm failed: ");
		return -1;
	}

	if (init_buf) {
		init_size = (size < init_buf_len) ? size : init_buf_len;
		memcpy(shm, init_buf, init_size);
	}

	shmdt(shm);

	return 0;
}



//-----------------------------------------------------------
// exported interface functions
//-----------------------------------------------------------

/*
 * @brief 创建一个初始化成 0 的 shm segment
 * 注意: 不允许创建 IPC_PRIVATE 类型的 shm (key不能是0);
 */
int tm_shm_create(key_t key, size_t size)
{
	return tm_shm_create_init(key, size, 0, 0);
}

/*
 * @brief 注意:
 * 如果 init_buf_len > size,
 * 		则仅把 init_buf 的前 init_buf_len字节 初始化到新创建的 shm 中;
 * 注意: 不允许创建 IPC_PRIVATE 类型的 shm (key不能是0);
 */
int tm_shm_create_init(key_t key, size_t size, void *init_buf, size_t init_buf_len)
{
	void *shm = 0;
	int init_size = 0;

	if (key == 0) {
		printf("error - forbid to create IPC_PRIVATE shm\n");
		return -1;
	}

	/* 尝试获取之前已经由相同的key建立起来的shm */
	int shmid = shmget(key, 0, 0);

	if (shmid == -1) {
		return do_tm_shm_create_init(key, size, init_buf, init_buf_len);
	}

	/*
	 * 之前已经创建了由key标识的shm, 则:
	 * 1. 如果 size 相同, 则不重复创建, 但:
	 * 		(1) 如果指定了 init_buf, 则用 init_buf 初始化该shm;
	 * 		(2) 如果没有指定 init_buf, 则 把该 shm 初始化成 0;
	 *
	 * 2. 如果 size 不相同, 则报错退出;
	 */
	struct shmid_ds shm_stat;
	if (shmctl(shmid, IPC_STAT, &shm_stat) == -1) {
		perror("error - stat shm fialed: ");
		return -1;
	}

	size_t elder_size = shm_stat.shm_segsz;
	if (elder_size == size) {
		shm = shmat(shmid, 0, 0);
		if (shm == (void *)-1) {
			perror("error - attach elder shm failed: ");
			return -1;
		}

		if (init_buf) {
			init_size = (size < init_buf_len) ? size : init_buf_len;
			memcpy(shm, init_buf, init_size);
		} else {
			memset(shm, 0, size);
		}

		shmdt(shm);
	} else {
		printf("error - create elder_size=%zd diff from new_size=%zd\n",
				elder_size, size);
		return -1;
	}

	return 0;
}

/*
 * @brief 创建一个初始化成 0 的 shm segment
 * 若在system-wide内有相同key的shm, 则返回错误
 */
int tm_shm_create_new(key_t key, size_t size)
{
	return tm_shm_create_new_init(key, size, 0, 0);
}

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
int tm_shm_create_new_init(key_t key, size_t size, void *init_buf, size_t init_buf_len)
{
	/* 检查在system-wide内是否有由相同的key建立起来的shm */
	if (shmget(key, 0, 0) != -1) {
		printf("error - create dup shm: key=0x%x\n", key);
		return -1;
	}

	return do_tm_shm_create_init(key, size, init_buf, init_buf_len);
}

/**
 * @brief attach 到 new_shmid 的 shm 上, 同时 detach old_shmid 的 shm;
 * 注意: 第一次 update_attach 的时候, 要给 shm_mgr->key 赋值;
 */
int tm_shm_update_attach(struct tm_shm_mgr_t *shm_mgr)
{
	key_t key = 0; 
	int shmid = 0;
	void *shm = 0;

	if (!shm_mgr || !shm_mgr->key) {
		printf("error - update shm: shm_mgr=%p, key=0x%x\n",
				shm_mgr, shm_mgr ? shm_mgr->key : 0);
		return -1;
	}

	key = shm_mgr->key;
	shmid = shmget(key, 0, 0);
	if (shmid == -1) {
		printf("error - update shmget failed\n");
		return -1;
	}

	struct shmid_ds shm_stat;
	if (shmctl(shmid, IPC_STAT, &shm_stat) == -1) {
		printf("error - update shmctl IPC_STAT failed\n");
		return -1;
	}

	if (shmid == shm_mgr->shmid) {
		if (shm_mgr->size != shm_stat.shm_segsz) {
			printf("error - update same shmid but diff shm_segsz[size:%lu, existed_size:%lu]\n", 
                    shm_mgr->size, shm_stat.shm_segsz);
			return -1;
		}
	}

	/* 新 shmid: 新shm */
	shm = shmat(shmid, 0, 0);
	if (shm == (void *)-1) {
		printf("error - update diff shmid but shmat failed\n");
		return -1;
	}

	/*
	 * 当第一次 update_attach 时,
	 * 由于 shm_mgr->shm == 0, 会导致 shmdt() 返回 -1,
	 * 但这不是错误, 因此忽略此错误
	 */
	shmdt(shm_mgr->shm);

	shm_mgr->shmid = shmid;
	shm_mgr->size = shm_stat.shm_segsz;
	shm_mgr->shm = shm;

	return 0;
}

/**
 * @brief 测试 system-wide 是否有 key 的 shm
 * @return 0: key 的 shm 还没建立; 1: key 的 shm 已经建立; -1: 出错
 */
int tm_shm_test_shm_exist(key_t key)
{
	/* 尝试获取之前已经由相同的key建立起来的shm */
	int shmid = shmget(key, 0, 0);
	if (shmid == -1) {
		/* 无法获取shmid, shm不存在 */
		return 0;
	}

	struct shmid_ds shm_stat;
	if (shmctl(shmid, IPC_STAT, &shm_stat) == -1) {
		/* 无法获取shm_stat, shm也不存在 */
		return 0;
	}

	return 1;
}
