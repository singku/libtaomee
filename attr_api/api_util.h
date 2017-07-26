#ifndef __API_UTIL_H__
#define __API_UTIL_H__

#include <stdint.h>

uint64_t make_attr_key(uint64_t attr, uint32_t pid);
int check_attr_key_matchable(uint64_t attr, uint64_t key);


#endif /* __API_UTIL_H__ */
