/*
** Copyright (C) 2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


/*
**   ACSMX.C
**
**
*/
#ifndef 	_ACSM_H
#define		_ACSM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
*   Prototypes
*/
#ifndef	ALPHABET_SIZE
#define	ALPHABET_SIZE    (256)
#endif

/*1MBytes buffer*/
#define	MAX_STR_BUFFER		(1000000)//待匹配串的最大长度
#define	ACSM_FAIL_STATE   	(-1)		//失效指针
#define	MAX_KEYWORD_LEN		(256)		//脏词最大长度
#define	MAX_PATTERNS		(81920)		//最大8192个脏词
#define	MAX_ADS_LEN			(4096)		//广告长度
#define	MAX_ADS				(2048)		//最大2048条广告
#define	MAX_STATE			(512 * 1024) //最多40W个节点 512*1k*1k＝512M

#define	PATTERNS_TABLE_LEN	(sizeof(ACSM_PATTERN) * MAX_PATTERNS)
#define	ADS_TABLE_LEN		0//(sizeof(ACSM_ADS) * MAX_ADS)		//扩展用
#define	STATE_TABLE_LEN		(sizeof(ACSM_STATETABLE) * MAX_STATE)

typedef struct _acsm_pattern {
	unsigned char		patrn[MAX_KEYWORD_LEN];			//这个模式是指变成大写的模式
	//unsigned char		casepatrn[MAX_KEYWORD_LEN];		//这个是大小写敏感的模式//扩展用
	int     			n;								//模式字符串长度
	//int     			nocase;							//扩展用
} ACSM_PATTERN;

typedef struct _acsm_pattern_ads {
	unsigned char		patrn[MAX_KEYWORD_LEN];			//这个模式是指变成大写的模式
	//unsigned char		casepatrn[MAX_KEYWORD_LEN];		//这个是大小写敏感的模式
	int     			n;								//模式字符串长度
	//int      			nocase;
} ACSM_ADS;

typedef struct  {	//状态结构体,用于状态确定,转移的构建,匹配状态的表示,失效函数表示

	/* Next state - based on input character */
	int      NextState[ ALPHABET_SIZE ];			//这个状态的下一状态表,最大256,被置位的下标代表状态转移的边(字符:256之内),对应的值代表下一状态号

	/* Failure state - used while building NFA & DFA  */
	int      FailState;			//失效函数值

	/* List of patterns that end here, if any */
	int		longest_pattern_index;   //这个状态能够完成匹配的pattern 最长的.

}ACSM_STATETABLE;


/*
* State machine Struct
*/
typedef struct {

	int acsmMaxStates;						//总状态数也就是总字符串长度,一个字符一个状态
	int acsmNumStates;						//状态机目前已分配状态数

	int total_pattern;
	int flag;									//标记该树可用与否，0表示用自己，1表示用对方(2颗匹配树)
	ACSM_PATTERN    *acsmPatterns;		//模式链表，第一个pattern在链表尾，最后一个在头部
	ACSM_STATETABLE *acsmStateTable;		//状态表

}ACSM_STRUCT;

typedef struct keyword_info
{
	char	key_str[MAX_KEYWORD_LEN];
	int		key_position;
}keyword_info;

/*
*   Prototypes
*/
ACSM_STRUCT * acsmNew();

void acsmNew_with_fd(int fd, ACSM_STRUCT **handle1, ACSM_STRUCT **handle2);

int acsmAddPattern( ACSM_STRUCT * p, unsigned char * pat, int n);
int acsmCompile ( ACSM_STRUCT * acsm );
int acsmSearch (ACSM_STRUCT * acsm, unsigned char *Tx, int n, int jump_tag); //返回查找到的个数
void acsmFree ( ACSM_STRUCT * acsm );
int acsm_get_first_dirty_word(ACSM_STRUCT * acsm, unsigned char *Tx, int n, unsigned char *retbuf, int jump_tag);

int acsm_pattern_replace(ACSM_STRUCT * acsm, unsigned char *Tx, int n, int jump_tag);


#endif			//_ACSM_H
