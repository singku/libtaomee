#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include "shm.h"
#include "api_util.h"
#include "hash_table.h"

#include "attr_api.h"


/* 注意: 一定要和 attr_push.c 的定义一致, 否则就乱套了 */
static uint32_t _nodes_num[] = {
	50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000,  
	50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000, 50000,  
};
static uint32_t _mods[] = {
	49999, 49993, 49991, 49957, 49943, 49939, 49937, 49927, 49921, 49919, 
	49891, 49877, 49871, 49853, 49843, 49831, 49823, 49811, 49807, 49801,
};
COMPILE_ASSERT(ARRAY_SIZE(_nodes_num) <= MAX_ATTR_HT_ROW_NUM);
COMPILE_ASSERT(ARRAY_SIZE(_nodes_num) == ARRAY_SIZE(_mods));
static uint32_t _row_num = ARRAY_SIZE(_nodes_num);

struct shm_mgr_t attr_shm_mgr, *attr_mgr = &attr_shm_mgr;
struct hash_table_t attr_nodes_hash_table, *attr_nodes_htab = &attr_nodes_hash_table;
uint64_t empty_key = 0;

uint64_t keypid = 0;

/* 指向上报量shm, 重启时置NULL, 遇到NULL就初始化 */
struct attr_node_t *attr_shm = NULL;


/* return: 0: node 的 key 与 key 相同; 1: node 的 key 与 key 不相同 */
static int attr_node_compare(const void *key, const void *node)
{
	uint64_t _key = *((uint64_t *)key);
	struct attr_node_t *_node = (struct attr_node_t*)node;
	return ((_key == _node->key) ? 0 : 1);
}

int get_attr_shm(void)
{
    uint32_t row_num = _row_num;
    uint32_t *nodes_num = _nodes_num;
    uint32_t *mods = _mods;

    void *table = NULL;
	key_t key = SHM_KEY_ATTR;
	struct hash_table_t *htab = attr_nodes_htab;
    size_t node_size = sizeof(struct attr_node_t);
    size_t size = calc_hash_table_size(node_size, row_num, nodes_num);

    init_shm_mgr(attr_mgr, key);
    get_shm_create_noinitexist(key, size, NULL, 0);
    if (update_shm_attach(attr_mgr) == -1) {
        fprintf(stderr, "Failed to update attr_mgr");
		return -1;
    }
    table = attr_mgr->shm;
    if (hash_table_init(htab, table, size, node_size, row_num, nodes_num, mods, attr_node_compare) == -1) {
        fprintf(stderr, "Failed to hash_table_init\n");
		return -1;
    }
    if (htab->total_node_num > MAX_HT_NODES_NUM) {
        fprintf(stderr, "hash_table (%u) over MAX_HT_NODES_NUM(%u)\n",
                htab->total_node_num, MAX_HT_NODES_NUM);
		return -1;
    }

	attr_shm = (struct attr_node_t *)(attr_mgr->shm);
    return 0;
}

/**
 * @attr: 上报ID
 * @val: 更新值
 * @method: 更新方式
 * @exist: attr是否已存在
 * @return -1: 失败, 0: 成功
 */
int update_attr(uint64_t attr, int64_t val, int method, int *exist)
{
	if (!attr_shm && get_attr_shm() == -1) {
		fprintf(stderr, "cannot get attr_shm!\n");
		return -1;
	}

	if (ATTR_UNLIKELY(keypid == 0)) {
		keypid = getpid();
	}
	uint64_t key = make_attr_key(attr, keypid);

	/* 查找 key 是否已存在 */
	int _exist = 0;
	struct attr_node_t *attr_node = HTAB_LOOKUP_EX(attr_nodes_htab, key, _exist);

	if (!attr_node) {
		fprintf(stderr, "attr out of memory!\n");
		return -1;
	}

	if (ATTR_UNLIKELY(_exist == 0)) {
		printf("[NEW ATTR_NODE]\tkey: %#lx, attr: %#lx, val: %ld\n",
				key, attr, val);
	}

	if (exist) *exist = _exist;

	/* 有这种可能: 多个进程同时获得了同一个空的attr_node,
	 * 这样会导致这个attr_node紊乱, 但这种可能性极小,
	 * 且push会检查出这种状况, 并清除这样的attr_node */
	if (ATTR_UNLIKELY(_exist == 0)) {
		attr_node->key = key;
		attr_node->attr = attr;
		attr_node->val = 0;
	}
	if (method == ATTR_UPDATE_METHOD_INCR) {
		attr_node->val += val;
	} else {
		attr_node->val = val;
	}

	return 0;
}

/**
 * @attr: 上报ID
 * @val: attr的当前值
 * @exist: 是否存在: 0: 不存在, 1: 存在; (exist用于区分没有get到和val是0的情况)
 * @return -1: 失败, 0: 成功(包括attr不存在)
 */
int get_attr(uint64_t key, uint64_t attr, int64_t *val, int *exist)
{
	if (!attr_shm && get_attr_shm() == -1) {
		fprintf(stderr, "cannot get attr_shm!\n");
		return -1;
	}

	struct attr_node_t *attr_node = HTAB_LOOKUP(attr_nodes_htab, key);
	if (!attr_node) {
		if (exist) *exist = 0;
		*val = 0;
		return 0;
	}

	if (exist) *exist = 1;
	*val = attr_node->val;
	return 0;
}

/**
 * @attr: 上报ID
 * @val: attr的当前值
 * @exist: 是否存在: 0: 不存在, 1: 存在; (exist用于区分没有get到和val是0的情况)
 * @return -1: 失败, 0: 成功(包括attr不存在)
 */
int get_self_attr(uint64_t attr, int64_t *val, int *exist)
{
	if (ATTR_UNLIKELY(keypid == 0)) {
		keypid = getpid();
	}
	uint64_t key = make_attr_key(attr, keypid);

	return get_attr(key, attr, val, exist);
}

#ifdef __FRAMEWORK_TEST_ATTR_API__

int main(int argc, char **argv)
{
#define GET_P_ATTR(__attr) \
	do { \
		int64_t __val = 0; \
		int __exist = 0; \
		GET_SELF_ATTR_EXIST(__attr, __val, __exist); \
		printf("[GET]\tattr: " #__attr ", exist: %s, val: %ld\n", \
				__exist ? "exist" : "noexist", __val); \
	} while(0)

#define INCR_P_ATTR(__attr, __val) \
	do { \
		int __exist = 0; \
		INCR_ATTR_EXIST(__attr, __val, __exist); \
		printf("[INCR]\tattr: %u, exist: %s, incr-val: %d\n", \
				__attr, __exist ? "exist" : "new", __val); \
	} while(0)

#define SET_P_ATTR(__attr, __val) \
	do { \
		int __exist = 0; \
		SET_ATTR_EXIST(__attr, __val, __exist); \
		printf("[SET]\tattr: %u, exist: %s, set-val: %d\n", \
				__attr, __exist ? "exist" : "new", __val); \
	} while(0)

	INCR_P_ATTR(10001, 100);
	INCR_P_ATTR(10001, 100);
	INCR_P_ATTR(10001, 100);

	GET_P_ATTR(10001);
	GET_P_ATTR(10002);

	INCR_P_ATTR(10002, 100);
	GET_P_ATTR(10002);

	SET_P_ATTR(10002, 142857);
	GET_P_ATTR(10002);

	INCR_P_ATTR(10003, 333);

	GET_P_ATTR(10001);
	GET_P_ATTR(10002);
	GET_P_ATTR(10003);


#undef GET_P_ATTR
#undef INCR_P_ATTR
#undef SET_P_ATTR
	return 0;
}

#endif // __FRAMEWORK_TEST_ATTR_API__
