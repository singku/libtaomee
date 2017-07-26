/**
 *============================================================
 *  @file      utils.h
 *  @brief     一些常用的函数。
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_UTILS_H_
#define LIBTAOMEE_UTILS_H_

/**
 * @def array_elem_num
 * @brief 计算数组arr_的元素个数。
 */
#ifdef  array_elem_num
#undef  array_elem_num
#endif
#define array_elem_num(arr_)     (sizeof(arr_) / sizeof((arr_)[0]))

/**
 * @def array_elem_size
 * @brief 计算数组arr_的每个元素占用内存的大小（byte）。
 */
#ifdef  array_elem_size
#undef  array_elem_size
#endif
#define array_elem_size(arr_)    sizeof((arr_)[0])

/**
 * @brief 得到libtaomee的版本号
 */
static inline const char*
libtaomee_get_version()
{
	return "0.7.6";
}

#endif // LIBTAOMEE_UTILS_H_
