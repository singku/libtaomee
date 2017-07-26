/*
 * utf8_punc.c
 *
 *	Created on:	2011-10-28
 * 	Author:		Singku
 *	Platform:	Linux 2.6.23 kernel x86-32/64
 *	Compiler:	GCC-4.1.2
 *	Copyright:	TaoMee, Inc. ShangHai CN. All Rights Reserved
 */


/*unicode的UTF8编码中关于全角字符,CJK标点,拉丁标点,和通用标点的定义及处理*/
/* http://www.unicode.org/charts */

#ifndef	_UTF8_PUNC_H
#define	_UTF8_PUNC_H

#include <stdint.h>

/*通用标点符号的UTF8编码范围,分两部分.需要跳过(3字节)*/
#define	GENERAL_PUNC_PART1_START		0xE28080
#define	GENERAL_PUNC_PART1_END			0xE280BF
#define	GENERAL_PUNC_PART2_START		0xE28180
#define	GENERAL_PUNC_PART2_END			0xE281AF

/*CJK(China Japan Korea)标点符号的UTF8编码范围.需要跳过(3字节)*/
#define	CJK_PUNC_START					0xE38080
#define	CJK_PUNC_END					0xE380BF

/*ASCII标点符号的UTF8编码范围.数字和字母不能跳过(1字节)*/
#define	ASCII_START						0x0
#define	ASCII_END						0x7F

/*ASCII码对应的全角符号UTF8编码范围,需要转换成单字节的ASCII码.再判断数字和字母(3字节)*/
#define	ASCII_SBC_PART1_START			0xEFBC81
#define	ASCII_SBC_PART1_END				0xEFBCBF
#define	ASCII_SBC_PART2_START			0xEFBD80
#define	ASCII_SBC_PART2_END				0xEFBD9F

/*拉丁小写标点的UTF8编码范围.跳过(3字节)*/
#define	SMALL_LATIN_PUNC_START			0xEFB990
#define	SMALL_LATIN_PUNC_END			0xEFB9AF

/*拉丁标点符号的UTF8编码范围.跳过(2字节)*/
#define	LATIN_PUNC_START				0xC2A0
#define	LATIN_PUNC_END					0xC2BF
#define	LATIN_PUNC_MULTIPLY				0xC397	/*乘法×*/
#define	LATIN_PUNC_DIVID				0xC3B7	/*除法÷*/

#define	ASCII_NUMBER_START				0x31
#define	ASCII_NUMBER_END				0x39
#define	ASCII_ALPHA_UPPER_START			0x41
#define	ASCII_ALPHA_UPPER_END			0x5A
#define	ASCII_ALPHA_LOWER_START			0x61
#define	ASCII_ALPHA_LOWER_END			0x7A

#define	IS_ASCII_NUMBER(c)			((c) >= 0x31 && (c) <= 0x39)
#define	IS_ASCII_UPPER_ALPHA(c)		((c) >= 0x41 && (c) <= 0x5A)
#define	IS_ASCII_LOWER_ALPHA(c)		((c) >= 0x61 && (c) <= 0x7A)
#define	IS_ASCII_ALPHA(c) 			((IS_ASCII_UPPER_ALPHA((c))) || (IS_ASCII_LOWER_ALPHA((c))))
#define	IS_ASCII_ALPHA_OR_NUMBER(c) ((IS_ASCII_NUMBER((c))) || (IS_ASCII_ALPHA((c))))
#define	IS_ASCII_PUNC_OR_SYMBOL(c)	((c) <= 0x7F && !(IS_ASCII_ALPHA_OR_NUMBER((c))))


/*
* @brief 将字符串s中的全角转换为半角,并跳过标点和符号,过滤后的串拷贝到d中
* @param flag = 0:default,to upperCase; flag = 1, just filter
* @param m记录源串s的长度,并根据过滤规则缩减为d的长度.
* @param flag 默认为0,表示过滤后的串中的字符换为大写.flag为1不转换.
* @return viod
*/
void string_filter(unsigned char * d, unsigned char *s, int *m, int flag);

/*
* @brief 将字符串s中的全角转换为半角,多余字节置'-',标点和符号置'+',小写转换为大写,结果串存入d
* @param int len:记录源串s的长度
* @param unsigned char *d: 目的串地址，需调用者提供
* @param unsigned char *s: 源串地址。
* @return void
*/
void string_replace(unsigned char * d, unsigned char *s, int len);

/**
 * @brief 将源字符串s中一对尖括号中的字符替换成字符'+'
 * 替换过后的内容放置于d中 (最多处理65536个连续开没有闭的括号)
 */
void replace_char_in_angle_brackets(unsigned char *d, unsigned char *s, int len);

/*定义unicode中全角字符(SBC)的范围以及向半角转换的函数*/
#define	UNICODE_DBC_SP			0x20
#define	UNICODE_DBC_START		0x21
#define 	UNICODE_DBC_END		0x7e
#define	UNICODE_SBC_SP			0x3000
#define	UNICODE_SBC_START		0xFF01
#define	UNICODE_SBC_END			0xFF5E
#define	SUB_SBC_DBC				0xFEE0

#define	UTF8_3_t				0xE0
#define	UTF8_2_t				0x80
#define	UTF8_1_t				0x80

typedef struct d_s_utf8 {
	uint8_t dbc;	//半角
	uint8_t sbc[3];	//全角
}d_s_utf8_t;

/* @brief 全角向半角转换
 * @param p: 存放全角或者半角的结构体缓冲区指针, flag:0 表示半角向全角转换，1表示全角向半角转换.
 * @return 0:成功，-1:flag非法或flag=0但对应的半角字符没有全角表示法
 */
int dbc_sbc_exchange(d_s_utf8_t *p, int flag);


/*
** Case Translation Table
*/
extern unsigned char xlatcase[256];

/*
* Init the acsm_xlatcase Table,Trans alpha to UpperMode
* Just for the NoCase State
*/
extern void init_xlatcase ();

#endif	/*_UTF8_PUNC_H*/

