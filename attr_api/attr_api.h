#ifndef __ATTR_API_H__
#define __ATTR_API_H__

#include <stdint.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* COMPILE ASSERT */
#define __CAT_TOKEN(t1,t2) t1##t2
#define CAT_TOKEN(t1,t2) __CAT_TOKEN(t1,t2)
#define COMPILE_ASSERT(x)  \
	enum { CAT_TOKEN (comp_assert_at_line_, __LINE__) = 1 / !!(x) };


/* likely and unlikely */
#define ATTR_LIKELY(x) __builtin_expect(!!(x), 1)
#define ATTR_UNLIKELY(x) __builtin_expect(!!(x), 0)



/* START: 这里设定都必须和 attr_push.h 中的一摸一样, 否则获取不到shm或者shm不配套 */

/* attr_shm 的shm_key */
#define SHM_KEY_ATTR					(1000000) /* 0x000f4240 */
/* 最多hash-row */
#define MAX_ATTR_HT_ROW_NUM				(100)
/* 最多允许多少个hash节点 */
#define MAX_HT_NODES_NUM				(1000000)

/* END: 这里设定都必须和 attr_api.h 中的一摸一样, 否则获取不到shm或者shm不配套 */


#define HTAB_LOOKUP(_htab, _attr) \
    hash_table_lookup_node(_htab, (void*)&_attr, _attr)

#define HTAB_LOOKUP_EX(_htab, _attr, _exist) \
	hash_table_lookup_node_ex(_htab, (void*)&empty_key, (void*)(&_attr), _attr, &_exist)

#define ATTR_UPDATE_METHOD_INCR			(0)
#define ATTR_UPDATE_METHOD_SET			(1)

/* 上报量节点结构 */
struct attr_node_t {
	/* hash-key: high32:pid + low32:attr-low32 */
	uint64_t		key;
	/* 0 表示没有占据 */
	uint64_t		attr;
	/* 分钟统计值 */
	int64_t			val;
};

/**
 * @attr: 上报ID
 * @val: 更新值
 * @method: 更新方式
 * @return -1: 失败, 0: 成功
 */
int update_attr(uint64_t attr, int64_t val, int method, int *exist);

/**
 * @attr: 上报ID
 * @val: attr的当前值
 * @exist: 是否存在: 0: 不存在, 1: 存在; (exist用于区分没有get到和val是0的情况)
 * @return -1: 失败, 0: 成功(包括attr不存在)
 */
int get_self_attr(uint64_t attr, int64_t *val, int *exist);

int get_attr(uint64_t key, uint64_t attr, int64_t *val, int *exist);
int get_self_attr(uint64_t attr, int64_t *val, int *exist);

#define INCR_ATTR(_attr, _val) \
	update_attr(_attr, _val, ATTR_UPDATE_METHOD_INCR, NULL)

#define SET_ATTR(_attr, _val) \
	update_attr(_attr, _val, ATTR_UPDATE_METHOD_SET, NULL)

#define GET_SELF_ATTR(_attr, _val) \
	get_self_attr(_attr, &_val, NULL)

#define INCR_ATTR_EXIST(_attr, _val, _exist) \
	update_attr(_attr, _val, ATTR_UPDATE_METHOD_INCR, &_exist)

#define SET_ATTR_EXIST(_attr, _val, _exist) \
	update_attr(_attr, _val, ATTR_UPDATE_METHOD_SET, &_exist)

#define GET_SELF_ATTR_EXIST(_attr, _val, _exist) \
	get_self_attr(_attr, &_val, &_exist)


#endif // __ATTR_API_H__
