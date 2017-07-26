/**
 *============================================================
 *  @file      bitmanip.h
 *  @brief     位操作函数
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_BITMANIP_H_
#define LIBTAOMEE_BITMANIP_H_

// C89
#include <assert.h>
#include <stddef.h>
// C99
#include <stdint.h>

/**
 * @brief 计算`val`中值为1的位的个数。
 * @param uint8_t val
 * @return int, `val`中值为1的位的个数。
 */
static inline int
nbits_on8(uint8_t val)
{
	val = ((val & 0xAA) >> 1) + (val & 0x55);
	val = ((val & 0xCC) >> 2) + (val & 0x33);
	val = ((val & 0xF0) >> 4) + (val & 0x0F);

	return val;
}

// 计算`val_`里面值为1的位的个数。
#define NBITS_ON(val_, nloops_, cnt_) \
		do { \
			int i = 0; \
			for (; i != (nloops_); ++i) { \
				(cnt_) += nbits_on8( *(((uint8_t*)&(val_)) + i) ); \
			} \
		} while (0)

/**
 * @brief 计算`val`中值为1的位的个数。
 * @param uint16_t val
 * @return int, `val`中值为1的位的个数。
 */
static inline int
nbits_on16(uint16_t val)
{
	int cnt = 0;
	NBITS_ON(val, 2, cnt);

	return cnt;
}

/**
 * @brief 计算`val`中值为1的位的个数。
 * @param uint32_t val
 * @return int, `val`中值为1的位的个数。
 */
static inline int
nbits_on32(uint32_t val)
{
	int cnt = 0;
	NBITS_ON(val, 4, cnt);

	return cnt;
}

/**
 * @brief 计算`val`中值为1的位的个数。
 * @param uint64_t val
 * @return int, `val`中值为1的位的个数。
 */
static inline int
nbits_on64(uint64_t val)
{
	int cnt = 0;
	NBITS_ON(val, 8, cnt);

	return cnt;
}

/**
 * @brief 把数组arr的第pos位设为1。arr[pos / 8] |= (1u << (pos % 8))。
 * @param uint8_t* arr
 * @param size_t len,  数组arr的长度。
 * @param int pos,  (pos / 8)必须小于len。
 * @return void
 */
static inline void
set_bit_on(uint8_t* arr, size_t len, int pos)
{
	assert(pos > 0);

	pos -= 1;

	int i = pos / 8;
	assert(i < len);

	arr[i] |= (1u << (pos % 8));
}

/**
 * @brief 把val的第pos位设为1。实参可以是uint8_t、uint16_t和uint32_t类型。
 * @param uint32_t val
 * @param int pos
 * @return uint32_t,  第pos位被设为1后的值。
 */
static inline uint32_t
set_bit_on32(uint32_t val, int pos)
{
	assert(pos > 0);

	return (val | (1u << (pos - 1)));
}

/**
 * @brief 把val的第pos位设为1。
 * @param uint64_t val
 * @param int pos
 * @return uint64_t,  第pos位被设为1后的值。
 */
static inline uint64_t
set_bit_on64(uint64_t val, int pos)
{
	assert(pos > 0);

	return (val | (1LLu << (pos - 1)));
}

/**
 * @brief 把数组arr的第pos位设为0。arr[pos / 8] &= ~(1u << (pos % 8))。
 * @param uint8_t* arr
 * @param size_t len,  数组arr的长度。
 * @param int pos,  (pos / 8)必须小于len。
 * @return void
 */
static inline void
set_bit_off(uint8_t* arr, size_t len, int pos)
{
	assert(pos > 0);

	pos -= 1;

	int i = pos / 8;
	assert(i < len);

	arr[i] &= ~(1u << (pos % 8));
}

/**
 * @brief 把val的第pos位设为0。实参可以是uint8_t、uint16_t和uint32_t类型。
 * @param uint32_t val
 * @param int pos
 * @return uint32_t,  第pos位被设为0后的值。
 */
static inline uint32_t
set_bit_off32(uint32_t val, int pos)
{
	assert(pos > 0);

	return (val & ~(1u << (pos - 1)));
}

/**
 * @brief 把val的第pos位设为0。
 * @param uint64_t val
 * @param int pos
 * @return uint64_t,  第pos位被设为0后的值。
 */
static inline uint64_t
set_bit_off64(uint64_t val, int pos)
{
	assert(pos > 0);

	return (val & ~(1LLu << (pos - 1)));
}

/**
 * @brief 检测arr的第pos位是否为1。arr[pos / 8] & (1u << (pos % 8))。
 * @param uint8_t* arr
 * @param size_t len,  数组arr的长度。
 * @param int pos,  (pos / 8)必须小于len。
 * @return int, 如果第pos为1，则返回1；反之，则返回0。
 */
static inline int
test_bit_on(uint8_t* arr, size_t len, int pos)
{
	assert(pos > 0);

	pos -= 1;

	int i = pos / 8;
	assert(i < len);

	return !!(arr[i] & (1u << (pos % 8)));
}

/**
 * @brief 检测val的第pos位是否为1。实参可以是uint8_t、uint16_t和uint32_t类型。
 * @param uint32_t val
 * @param int pos
 * @return int,  如果第pos为1，则返回1；反之，则返回0。
 */
static inline int
test_bit_on32(uint32_t val, int pos)
{
	assert(pos > 0);

	return !!(val & (1u << (pos - 1)));
}

/**
 * @brief 检测val的第pos位是否为1。
 * @param uint64_t val
 * @param int pos
 * @return int,  如果第pos为1，则返回1；反之，则返回0。
 */
static inline int
test_bit_on64(uint64_t val, int pos)
{
	assert(pos > 0);

	return !!(val & (1LLu << (pos - 1)));
}

#endif // LIBTAOMEE_BITMANIP_H_
