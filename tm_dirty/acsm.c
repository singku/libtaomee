/*
**
** Multi-Pattern Search Engine
**
** Aho-Corasick State Machine -  uses a Deterministic Finite Automata - DFA
**
** Copyright (C) 2002 Sourcefire,Inc.
** Marc Norton
**
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
**
**
**   Reference - Efficient String matching: An Aid to Bibliographic Search
**               Alfred V Aho and Margaret J Corasick
**               Bell Labratories
**               Copyright(C) 1975 Association for Computing Machinery,Inc
**
**   Implemented from the 4 algorithms in the paper by Aho & Corasick
**   and some implementation ideas from 'Practical Algorithms in C'
**
**   Notes:
**     1) This version uses about 1024 bytes per pattern character - heavy  on the memory.
**     2) This algorithm finds all occurrences of all patterns within a
**        body of text.
**     3) Support is included to handle upper and lower case matching.
**     4) Some comopilers optimize the search routine well, others don't, this makes all the difference.
**     5) Aho inspects all bytes of the search text, but only once so it's very efficient,
**        if the patterns are all large than the Modified Wu-Manbar method is often faster.
**     6) I don't subscribe to any one method is best for all searching needs,
**        the data decides which method is best,
**        and we don't know until after the search method has been tested on the specific data sets.
**
**
**  May 2002  : Marc Norton 1st Version
**  June 2002 : Modified interface for SNORT, added case support
**  Aug 2002  : Cleaned up comments, and removed dead code.
**  Nov 2,2002: Fixed queue_init() , added count=0
**
**  Wangyao : wangyao@cs.hit.edu.cn
**
**  Apr 24,2007: WangYao Combined Build_NFA() and Convert_NFA_To_DFA() into Build_DFA();
**				 And Delete Some redundancy Code
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include <libtaomee/log.h>

#include "utf8_punc.h"
#include "acsm.h"

/*
* Malloc the AC Memory
*/

static void *AC_MALLOC (int n)
{
	void *p;
	p = malloc (n);
	return p;
}

/*
*Free the AC Memory
*/
static void AC_FREE (void *p)
{
	if (p)
		free (p);
}


/*
*    Simple QUEUE NODE
*/
typedef struct _qnode
{
	int state;
	struct _qnode *next;
}QNODE;

/*
*    Simple QUEUE Structure
*/
typedef struct _queue
{
	QNODE * head, *tail;
	int count;
}QUEUE;

/*
*Init the Queue
*/
static void queue_init (QUEUE * s)
{
	s->head = s->tail = 0;
	s->count = 0;
}


/*
*  Add Tail Item to queue
*/
static int queue_add (QUEUE * s, int state)
{
	QNODE * q;
	/*Queue is empty*/
	if (!s->head) {
		q = s->tail = s->head = (QNODE *) AC_MALLOC (sizeof (QNODE));
		/*if malloc failed,exit the problom*/
		if (q == NULL)
			ERROR_RETURN(("malloc failed when building ac trie"), -1);
		q->state = state;
		q->next = 0; /*Set the New Node's Next Null*/
	} else {
		q = (QNODE *) AC_MALLOC (sizeof (QNODE));
		if (q == NULL)
			ERROR_RETURN(("malloc failed when building ac trie"), -1);
		q->state = state;
		q->next = 0;
		/*Add the new Node into the queue*/
		s->tail->next = q;
		/*set the new node is the Queue's Tail*/
		s->tail = q;
	}
	s->count++;
	return 0;
}


/*
*  Remove Head Item from queue
*/
static int queue_remove (QUEUE * s)
{
	int state = 0;
	QNODE * q;
	/*Remove A QueueNode From the head of the Queue*/
	if (s->head) {
		q = s->head;
		state = q->state;
		s->head = s->head->next;
		s->count--;

		/*If Queue is Empty,After Remove A QueueNode*/
		if (!s->head) {
			s->tail = 0;
			s->count = 0;
		}
		/*Free the QueNode Memory*/
		AC_FREE (q);
	}
	return state;
}


/*
*Return The count of the Node in the Queue
*/
static int queue_count (QUEUE * s)
{
	return s->count;
}


/*
*Free the Queue Memory
*/
static void queue_free (QUEUE * s)
{
	while (queue_count (s)) {
		queue_remove (s);
	}
}

/*
*  Add a pattern to the list of patterns terminated at this state.
*  Insert at front of list.
*  px,pattern
*/
static void AddMatchListEntry (ACSM_STRUCT * acsm, int state, int pattern_index)
{
	int cur_index = acsm->acsmStateTable[state].longest_pattern_index;
	if ((cur_index < 0 )
		|| (acsm->acsmPatterns[cur_index].n < acsm->acsmPatterns[pattern_index].n)) {
		acsm->acsmStateTable[state].longest_pattern_index = pattern_index;
	}
}

/*
* Add Pattern States
*/
static int AddPatternStates (ACSM_STRUCT * acsm, int pattern_index)
{
	unsigned char *pattern;
	ACSM_PATTERN *p;
	int state = 0, next, n;
	p = &(acsm->acsmPatterns[pattern_index]);
	n = p->n; /*The number of alpha in the pattern string*/
	pattern = p->patrn;

   /*
	*  Match up pattern with existing states
	*/
	for (; n > 0; pattern++, n--) {
		next = acsm->acsmStateTable[state].NextState[*pattern];
		if (next == ACSM_FAIL_STATE)
			break;
		state = next;
	}
   /*
	*   Add new states for the rest of the pattern bytes, 1 state per byte
	*/
	for (; n > 0; pattern++, n--) {
		acsm->acsmNumStates++;
		if (acsm->acsmNumStates >= acsm->acsmMaxStates)
			ERROR_RETURN(("patterns state table is running out"), -1);
		acsm->acsmStateTable[state].NextState[*pattern] = acsm->acsmNumStates;
		state = acsm->acsmNumStates;
	}
	/*Here,An accept state,just add into the MatchListof the state*/
	AddMatchListEntry (acsm, state, pattern_index);

	return 0;
}


/*
*   Build Non-Deterministic Finite Automata
*/
static int Build_DFA (ACSM_STRUCT * acsm)
{
	int r, s;
	int i;
	QUEUE q, *queue = &q;

	/* Init a Queue */
	queue_init (queue);

	/* Add the state 0 transitions 1st */
	/*1st depth Node's FailState is 0, fail(x)=0 */
	//对于0状态的每个存在的子节点，其失效指针必然是0状态本身
	for (i = 0; i < ALPHABET_SIZE; i++) {
		s = acsm->acsmStateTable[0].NextState[i];
		if (s != ACSM_FAIL_STATE && s != 0) {
			if (queue_add (queue, s) == -1)			//0状态的所有子节点入队列
				return -1;
			acsm->acsmStateTable[s].FailState = 0;
		}
	}

	/* Build the fail state transitions for each valid state */
	while (queue_count (queue) > 0) {
		r = queue_remove (queue);

		/* Find Final States for any Failure */
		for (i = 0; i < ALPHABET_SIZE; i++) {
			int fs, next;
			/*** Note NextState[i] is a const variable in this block ***/
			if ((s = acsm->acsmStateTable[r].NextState[i]) != ACSM_FAIL_STATE) {
				//对于某个状态节点r，判断其每个子节点是否存在，若存在则从父节点依次追溯失效指针 直到找到或者到达ROOT节点
				if (queue_add (queue, s) == -1)
					return -1;
				fs = acsm->acsmStateTable[r].FailState;		//fs记录r的失效指针
				/*
				*  Locate the next valid state for 'i' starting at s
				*/
				/**** Note the  variable "next" ****/
				/*** Note "NextState[i]" is a const variable in this block ***/
				while ((next=acsm->acsmStateTable[fs].NextState[i]) == ACSM_FAIL_STATE) {
					//追溯失效指针(若fs的字状态[i]不存在，则查找fs的失效指针的子状态[i])
					//找到则定位到了next，若一直没有找到，则fs最终会等于0状态节点
					//而0状态的所有不存在的子节点都初始化成0了。所以while循环会退出
					//next=acsm->acsmStateTable[0].NextState[i] == 0 !=ACSM_FAIL_STATE
					fs = acsm->acsmStateTable[fs].FailState;
				}

				/*
				*  Update 's' state failure state to point to the next valid state
				*/

				//如果失效指针也有匹配列表，则匹配到本节点的时候，也应该匹配失效指针的匹配列表
				//如:she he两个模式 she的e的失效指针是he的e 所以匹配she时 必然也匹配了he
				acsm->acsmStateTable[s].FailState = next;
				/*
				ACSM_PATTERN* pat = acsm->acsmStateTable[next].MatchList;
				for (; pat != NULL; pat = pat->next) {
					AddMatchListEntry(acsm, s, pat);
                }
                */
				if (acsm->acsmStateTable[next].longest_pattern_index != -1) {
					AddMatchListEntry(acsm, s, acsm->acsmStateTable[next].longest_pattern_index);
				}
			} else {
				//如果r的某个子节点状态不存在(不在模式串中)，则在匹配的时候读入该字符应该失效
				//失效时应该回退到哪里呢？回退到失效指针的NextState[i];要么存在 要么为0
				//比如she和her,当串为sher时，she读入r会失效，则由she的e状态指向her的e的子状态r
				//如果没有her只有He则e的nextState[i]=0定位到root节点
				acsm->acsmStateTable[r].NextState[i] =
					acsm->acsmStateTable[acsm->acsmStateTable[r].FailState].NextState[i];
			}
		}
	}

	/* Clean up the queue */
	queue_free (queue);

	return 0;
}

void acsmNew_with_fd(int fd, ACSM_STRUCT **handle1, ACSM_STRUCT **handle2)
{
	char *p;
	init_xlatcase();
	int len = sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN + STATE_TABLE_LEN;

	ftruncate(fd, len*2);
	p = mmap(NULL, len*2, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);

	if (p == MAP_FAILED) {
		ERROR_LOG("mmap failed!:%s\n", strerror(errno));
	}

	ACSM_STRUCT *acsm;
	*handle1 = (ACSM_STRUCT*)p;
	acsm = *handle1;
	acsm->acsmPatterns = (ACSM_PATTERN*)(p + sizeof(ACSM_STRUCT));
	acsm->acsmStateTable = (ACSM_STATETABLE*)(p + sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN);

	*handle2 = (ACSM_STRUCT*)(p+len);
	acsm = *handle2;
	acsm->acsmPatterns = (ACSM_PATTERN*)(p +len+ sizeof(ACSM_STRUCT));
	acsm->acsmStateTable = (ACSM_STATETABLE*)(p+len + sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN);
}



/*
* Init the acsm DataStruct
*/
ACSM_STRUCT * acsmNew ()
{
	char *p;
	init_xlatcase ();
	p = mmap(NULL, sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN + STATE_TABLE_LEN,
			PROT_READ|PROT_WRITE,	MAP_SHARED|MAP_ANONYMOUS, -1, 0);// (sizeof (ACSM_STRUCT));
	if ((char*)p == (char*)-1) {
		ERROR_RETURN(("mmap failed!:%s\n", strerror(errno)), NULL);
	}

	ACSM_STRUCT *acsm = (ACSM_STRUCT*)p;
	acsm->acsmPatterns = (ACSM_PATTERN*)(p + sizeof(ACSM_STRUCT));
	acsm->acsmStateTable = (ACSM_STATETABLE*)(p + sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN);
	return acsm;
}


/*
*   Add a pattern to the list of patterns for this state machine
*/
int acsmAddPattern (ACSM_STRUCT * p, unsigned char *pat, int n)
{
	if (p->total_pattern >= MAX_PATTERNS) {
		ERROR_RETURN(("added patterns reach MAX_PATTERN"), -1);
	}

	int pattern_index = (p->total_pattern)++;
	ACSM_PATTERN *plist = &(p->acsmPatterns[pattern_index]);

	int new_n = (n > (MAX_KEYWORD_LEN - 1)) ?(MAX_KEYWORD_LEN - 1) :n;	//last BYTE SET TO O
	string_filter(plist->patrn, pat, &new_n, 0);
    if (new_n == 0) {
        p->total_pattern--;
        return 0;
    }
	memset(plist->patrn + new_n , 0, 1);
	
	plist->n = new_n;

	return 0;
}

/*
*   Compile State Machine
*/
int acsmCompile (ACSM_STRUCT * acsm)
{
	int i, k;
	ACSM_PATTERN * plist;

	/* Count number of states */
	acsm->acsmMaxStates = 1; /*State 0*/

	for (i = 0; i < acsm->total_pattern; i++) {
		plist = &(acsm->acsmPatterns[i]);
		acsm->acsmMaxStates += plist->n;//总共有多少状态，每个模式串长度的累加 再加上0状态
	}

    if (acsm->acsmMaxStates > MAX_STATE) {
        ERROR_RETURN(("patterns's states reach MAX"), -1);
    }
	/* Initialize state zero as a branch */
	acsm->acsmNumStates = 0;

	/* Initialize all States NextStates to FAILED */
	for (k = 0; k < acsm->acsmMaxStates; k++) {
		acsm->acsmStateTable[k].longest_pattern_index = -1;
		for (i = 0; i < ALPHABET_SIZE; i++) {
			acsm->acsmStateTable[k].NextState[i] = ACSM_FAIL_STATE; //所有状态的next状态都置-1
		}
	}

	/* This is very import */
	/* Add each Pattern to the State Table */
	//用模式串初始化状态表，如果某个状态r的nextState[i]不存在 则依然是-1;
	for (i = 0; i < acsm->total_pattern; i++) {
		if (AddPatternStates (acsm, i) == -1)
			ERROR_RETURN(("acsm compiling failed: add pattern state failed"), -1);
	}

	/* Set all failed state transitions which from state 0 to return to the 0'th state */
	for (i = 0; i < ALPHABET_SIZE; i++) {
		if (acsm->acsmStateTable[0].NextState[i] == ACSM_FAIL_STATE) {
			acsm->acsmStateTable[0].NextState[i] = 0;//设置0状态所有不存在的子节点状态的下一个状态为0状态
		}
	}

	/* Build the NFA  */
	if (Build_DFA (acsm) == -1)
		ERROR_RETURN(("acsm compiling failed: build DFA failed"), -1);

	return 0;
}


/*64KB Memory*/

/*
*   Search Text or Binary Data for Pattern matches
*/
int acsmSearch (ACSM_STRUCT * acsm, unsigned char *Tx, int n, int jump_tag)
{
	int state;
	unsigned char Tc[MAX_STR_BUFFER];//1M,even use utf8 with 4bytes one symbol, this buffer can hold 250K symbol
    unsigned char Tc_1[MAX_STR_BUFFER];
	//ACSM_PATTERN * mlist;
	unsigned char *Tend;
	ACSM_STATETABLE * StateTable = acsm->acsmStateTable;
	//int nlen = 0; /*length of the found(matched) patten string*/
	//int keyword_index=0;
	unsigned char *T;
	//int index;

	//(*keyword_count) = 0;
	/*case convert and string filter */
	if (n > MAX_STR_BUFFER)
		ERROR_RETURN(("checked string is too long(MAX_STR_BUFFER=%u)", MAX_STR_BUFFER), -1);
	int new_n = n;
    if (jump_tag != 0) {
        replace_char_in_angle_brackets(Tc_1, Tx, n);
        string_filter(Tc, Tc_1, &new_n, 0);
    } else {
	    string_filter(Tc, Tx, &new_n, 0);
    }
	T = Tc;
	Tend = T + new_n;

	for (state = 0; T < Tend; T++) {	
		state = StateTable[state].NextState[*T];
		//如果StateTable[state].NextState[*T]在模式串中,则继续判断是否某个状态的结尾(有输出列表)
		//如果不在模式传中,则会失效,由于之前已经设置了失效指针，则会自动定位到下一个状态或者回到0状态。
		if( StateTable[state].longest_pattern_index != -1 ) {
			return 3;
			//以下代码可用在脏词替换中.待扩展
			#if 0
			for( mlist=StateTable[state].MatchList; mlist != NULL; mlist = mlist->next ) {
				/*Get the index  of the Match Pattern String in  the Text*/
				if(keyword_index >= 256) 
					return 0;

				index = T - mlist->n + 1 - Tc;
				nlen = strlen((const char *)mlist->casepatrn);
			
				(*keyword_count)++;
				keyword[keyword_index].key_position = index;
				//memset(keyword[keyword_index].key_str,0,sizeof(keyword[keyword_index].key_str));
				nlen = (nlen <= MAX_KEYWORD_LEN) ?nlen :MAX_KEYWORD_LEN;
				strncpy(keyword[keyword_index].key_str,(const char *)mlist->casepatrn, nlen);

				if(nlen == MAX_KEYWORD_LEN)
					keyword[keyword_index].key_str[MAX_KEYWORD_LEN - 1] = '\0';
				else
					keyword[keyword_index].key_str[nlen]='\0';

				keyword_index++;
					//T+=(nlen-1);
				//fprintf (stdout, "Match KeyWord %s at %d char\n", mlist->patrn,index);
			}
			#endif
		}
	}
	//just A-Z a-z
	T = Tc;
	Tend = T + new_n;
	for (state = 0; T < Tend; T++) {
		if (!IS_ASCII_ALPHA(*T))
			continue;
		state = StateTable[state].NextState[*T];
		if (StateTable[state].longest_pattern_index != -1) {
			return 1;
		}
	}

	//exclude A-Z a-z
	T = Tc;
	Tend = T + new_n;
	for (state = 0; T < Tend; T++) {
			if (IS_ASCII_ALPHA(*T))
					continue;
			state = StateTable[state].NextState[*T];
			if (StateTable[state].longest_pattern_index != -1) {
					return 2;
			}
	}

	return 0;
}

/*
*   Search Text or Binary Data for Pattern matches
*/
int acsm_get_first_dirty_word(ACSM_STRUCT * acsm, unsigned char *Tx, int n, unsigned char *retbuf, int jump_tag)
{
	int state;
	unsigned char Tc[MAX_STR_BUFFER];//1M,even use utf8 with 4bytes one symbol, this buffer can hold 250K symbol
    unsigned char Tc_1[MAX_STR_BUFFER];
	//ACSM_PATTERN * mlist;
	unsigned char *Tend;
	ACSM_STATETABLE * StateTable = acsm->acsmStateTable;
	//int nlen = 0; /*length of the found(matched) patten string*/
	//int keyword_index=0;
	unsigned char *T;
	//int index;

	//(*keyword_count) = 0;
	/*case convert and string filter */
	if (n > MAX_STR_BUFFER)
		ERROR_RETURN(("checked string is too long(MAX_STR_BUFFER=%u)", MAX_STR_BUFFER), -1);
	int new_n = n;
    if (jump_tag != 0) {
        replace_char_in_angle_brackets(Tc_1, Tx, n);
        string_filter(Tc, Tc_1, &new_n, 0);
    } else {
	    string_filter(Tc, Tx, &new_n, 0);
    }
	T = Tc;
	Tend = T + new_n;

	for (state = 0; T < Tend; T++) {	
		state = StateTable[state].NextState[*T];
		//如果StateTable[state].NextState[*T]在模式串中,则继续判断是否某个状态的结尾(有输出列表)
		//如果不在模式传中,则会失效,由于之前已经设置了失效指针，则会自动定位到下一个状态或者回到0状态。
		if( StateTable[state].longest_pattern_index != -1 ) {
            if (retbuf != NULL) {
                memcpy(retbuf, acsm->acsmPatterns[StateTable[state].longest_pattern_index].patrn, MAX_KEYWORD_LEN);
            }
			return 3;
	    }
    }
	//just A-Z a-z
	T = Tc;
	Tend = T + new_n;
	for (state = 0; T < Tend; T++) {
		if (!IS_ASCII_ALPHA(*T))
			continue;
		state = StateTable[state].NextState[*T];
		if (StateTable[state].longest_pattern_index != -1) {
            if (retbuf != NULL) {
                memcpy(retbuf, acsm->acsmPatterns[StateTable[state].longest_pattern_index].patrn, MAX_KEYWORD_LEN);
            }
			return 1;
		}
	}

	//exclude A-Z a-z
	T = Tc;
	Tend = T + new_n;
	for (state = 0; T < Tend; T++) {
			if (IS_ASCII_ALPHA(*T))
					continue;
			state = StateTable[state].NextState[*T];
			if (StateTable[state].longest_pattern_index != -1) {
                if (retbuf != NULL) {
                    memcpy(retbuf, acsm->acsmPatterns[StateTable[state].longest_pattern_index].patrn, MAX_KEYWORD_LEN);
                }
				return 2;
			}
	}

	return 0;
}


/*
* @brief 将源串Tx中的脏词全部替换成*，改变了Tx的内容
* @param ACSM_STRUCT *acsm: acsm句柄
* @param unsigned char *Tx: 待替换串地址.
* @param int n:替换串长度.
* @param jump_tag:是否跳过标签<>的检查
* @return 0:干净串,1:替换过.
*/
int acsm_pattern_replace(ACSM_STRUCT * acsm, unsigned char *Tx, int n, int jump_tag)
{
	int state;
	unsigned char Tc[3][MAX_STR_BUFFER];
    unsigned char Tc_1[MAX_STR_BUFFER];
	//1M,even use utf8 with 4bytes one symbol, this buffer can hold 250K symbol

	unsigned char *Tend;
	ACSM_STATETABLE * StateTable = acsm->acsmStateTable;

	unsigned char *T;

	if (n > MAX_STR_BUFFER)
		ERROR_RETURN(("checked string is too long(MAX_MSG_LEN=%u)", MAX_STR_BUFFER), -1);
 
    //先把标签换成+
    if (jump_tag != 0) {
        replace_char_in_angle_brackets(Tc_1, Tx, n);
        string_replace(Tc[0], Tc_1, n);
    } else {
	    string_replace(Tc[0], Tx, n);
    }

	memcpy(Tc[1], Tc[0], n);
	memcpy(Tc[2], Tc[0], n);

	int pattern_length;
	int flag = 0;
	int i,j;
	unsigned char *tmp;
	//全匹配
	T = Tc[0];
	Tend = T + n;
	for (state = 0, i = 0; T < Tend; T++, i++) {
		state = StateTable[state].NextState[*T];
		//如果StateTable[state].NextState[*T]在模式串中,则继续判断是否某个状态的结尾(有输出列表)
		//如果不在模式传中,则会失效,由于之前已经设置了失效指针，则会自动定位到下一个状态或者回到0状态。
		if( StateTable[state].longest_pattern_index != -1 ) {
			pattern_length = acsm->acsmPatterns[StateTable[state].longest_pattern_index].n;

			if (T+1 < Tend  && *(T+1) == '-') {
				T += 1;
				if (T+1 < Tend && *(T+1) == '-')
					T += 1;
			}

			tmp = T;
			for (j = 0; j < pattern_length; ) {
                if (*tmp != '-' || *tmp == '*') {
                    j++; //非'-'是模式字符
                }
				*tmp = '*';
				tmp--;
			}
			flag = 1;
		}//if
	}//for

	//just A-Z a-z
	T = Tc[1];
	Tend = T + n;
	for (state = 0, i = 0; T < Tend; T++, i++) {
		if (!IS_ASCII_ALPHA(*T))
			continue;

		state = StateTable[state].NextState[*T];
		//如果StateTable[state].NextState[*T]在模式串中,则继续判断是否某个状态的结尾(有输出列表)
		//如果不在模式传中,则会失效,由于之前已经设置了失效指针，则会自动定位到下一个状态或者回到0状态。
		if( StateTable[state].longest_pattern_index != -1 ) {
			pattern_length = acsm->acsmPatterns[StateTable[state].longest_pattern_index].n;

			if (T+1 < Tend  && *(T+1) == '-') {
				T += 1;
				if (T+1 < Tend && *(T+1) == '-')
					T += 1;
			}

			tmp = T;
			for (j = 0; j < pattern_length; ) {
				if (IS_ASCII_ALPHA(*tmp) || *tmp == '*')
					j++;
				if (IS_ASCII_ALPHA(*tmp)) {
					*tmp = '*';
				} else if (*tmp == '-'){
					if ((tmp-1 >= Tc[1] && IS_ASCII_ALPHA(*(tmp-1)))
						|| (tmp-2 >= Tc[1] &&IS_ASCII_ALPHA(*(tmp-2))))
						*tmp = '*';
				}
				tmp--;
			}
			flag = 1;
		}//if
	}//for

	//exclude A-Z a-z 0-9
	T = Tc[2];
	Tend = T + n;

	for (state = 0, i = 0; T < Tend; T++, i++) {
		if (IS_ASCII_ALPHA_OR_NUMBER(*T) || *T == '+' || *T == '-')
			continue;

		state = StateTable[state].NextState[*T];
		//如果StateTable[state].NextState[*T]在模式串中,则继续判断是否某个状态的结尾(有输出列表)
		//如果不在模式传中,则会失效,由于之前已经设置了失效指针，则会自动定位到下一个状态或者回到0状态。
		if( StateTable[state].longest_pattern_index != -1 ) {
			pattern_length = acsm->acsmPatterns[StateTable[state].longest_pattern_index].n;

			tmp = T;
			for (j = 0; j < pattern_length; ) {
				if ((!IS_ASCII_ALPHA_OR_NUMBER(*tmp) && *tmp != '+' && *tmp != '-') || *tmp == '*' ) {
					*tmp = '*';
					j++;
				}
				tmp--;
			}
			flag = 1;
		}//if
	}//for

	//merge *
	for (i = 0 ; i < n; i++) {
		if (Tc[0][i] == '*' || Tc[1][i] == '*' || Tc[2][i] == '*')
			Tx[i] = '*';
	}

	return flag;
}

/*
*   Free all memory
*/
void acsmFree (ACSM_STRUCT * acsm)
{
	if (munmap(acsm, sizeof(ACSM_STRUCT)) == -1)
		ERROR_LOG("mumap failed:%s", strerror(errno));
}
