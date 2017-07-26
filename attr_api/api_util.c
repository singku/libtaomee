
#include "api_util.h"



/** @return: key */
uint64_t make_attr_key(uint64_t attr, uint32_t pid)
{
	uint64_t attr_h8 = attr & 0xFF00;
	uint64_t attr_l8 = attr & 0xFF;
	uint64_t pid_h8 = pid & 0xFF00;
	uint64_t pid_l8 = pid & 0xFF;
	uint64_t key_h16 = ((attr_h8 ^ pid_h8) | (attr_l8 | pid_l8)) & 0xFFFF;
	uint64_t key_l16 = ((attr_h8 | pid_h8) | (attr_l8 ^ pid_l8)) & 0xFFFF;
	uint64_t key_l32 = (key_h16 << 16) | key_l16;

	/* key = attr.l32 | hash_l32(attr, pid) */
	return (attr << 32) | (key_l32 & 0xFFFFFFFF);
}

/** @return: 1: 匹配, 0: 不匹配 */
int check_attr_key_matchable(uint64_t attr, uint64_t key)
{
	/* 简单检查一下 attr 的低32位 和 key的高32位 */
	uint64_t attr_l32 = (attr & 0xFFFFFFFF);
	uint64_t key_h32 = (key >> 32);
	
	return (key_h32 == attr_l32) ? 1 : 0;
}
