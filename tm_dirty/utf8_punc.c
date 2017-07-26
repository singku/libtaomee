/*
 * utf8_punc.c
 *
 *	Created on:	2011-10-28
 * 	Author:		Singku
 *	Platform:	Linux 2.6.23 kernel x86-32/64
 *	Compiler:	GCC-4.1.2
 *	Copyright:	TaoMee, Inc. ShangHai CN. All Rights Reserved
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utf8_punc.h"
/*
** Case Translation Table
*/
unsigned char xlatcase[256];

/*
* Init the acsm_xlatcase Table,Trans alpha to UpperMode
* Just for the NoCase State
*/
void init_xlatcase ()
{
	int i;
	for (i = 0; i < 256; i++) {
		xlatcase[i] = toupper (i);
	}
}

/* @brief 全角向半角转换
 * @param p: 存放全角或者半角的结构体缓冲区指针, flag:0 表示半角向全角转换，1表示全角向半角转换.
 * @return 0:成功，-1:flag非法或flag=0但对应的半角字符没有全角表示法
 */
int dbc_sbc_exchange(d_s_utf8_t *p, int flag)
{	
	uint16_t unicode;
	if (flag == 0){
		if (p->dbc == UNICODE_DBC_SP)
			unicode = UNICODE_SBC_SP;
		else if (p->dbc >= UNICODE_DBC_START && p->dbc <= UNICODE_DBC_END)
			unicode = p->dbc + SUB_SBC_DBC;
		else 
			return -1;

		uint8_t hp = (unicode >> 12) & 0x000F;
		uint8_t mp = (unicode >> 6) &0x003F;
		uint8_t lp = (unicode & 0x003F);
		p->sbc[0] = UTF8_3_t | hp;
		p->sbc[1] = UTF8_2_t | mp;
		p->sbc[2] = UTF8_1_t | lp;
		
		return 0;
	} else if (flag == 1) {
		uint16_t cd = 0;
		cd = p->sbc[0] & 0x0F;
		unicode = cd << 12;
		cd = p->sbc[1] & 0x3F;
		unicode |= (cd << 6);
		cd = p->sbc[2] & 0x3F;
		unicode |= cd;
		if (unicode == UNICODE_SBC_SP)
			p->dbc = UNICODE_DBC_SP;
		else
			p->dbc = unicode - SUB_SBC_DBC;

		return 0;
	} else
		return -1;	
}

/*
* @brief 将字符串s中的全角转换为半角,并跳过标点和符号,过滤后的串拷贝到d中
* @param flag = 0:default,to upperCase; flag = 1, just filter
* @param m记录源串s的长度,并根据过滤规则缩减为d的长度.
* @param flag 默认为0,表示过滤后的串中的字符换为大写.flag为1不转换.
* @return viod
*/
void string_filter(unsigned char * d, unsigned char *s, int *m, int flag)
{
	int i,j;
	int len = *m;
	d_s_utf8_t convert;
	int sbc_flag;
	int jump_bytes;
	
	for (i = 0, j = 0; i < len; i++)
	{
		sbc_flag = 0;
		jump_bytes = 0;
		switch (s[i]) {
			case 0xC2:{/*latin punctuation 2Bytes*/
				if ( i+1 < len && s[i+1] >= 0xA0 && s[i+1] <= 0xBF) {
					jump_bytes = 1;
				}
				break;
			}
			case 0xC3:{/*multiple and divid 2Bytes*/
				if ( i+1 < len && (s[i+1] == 0x97 || s[i+1] == 0xB7)) {
					jump_bytes = 1;
				}
				break;
			}
			case 0xE2:{/*general punctuation*/
				if ( 	(i+2 < len) 
					&& (	(s[i+1] == 0x80 && s[i+2] >= 0x80 && s[i+2] <= 0xBF)
						||(s[i+1] == 0x81 && s[i+2] >= 0x80 && s[i+2] <= 0xAF)
					)
				)
					jump_bytes = 2;
				break;
			}
			case 0xE3: {/*cjk punctuation*/
				if ( i+2 < len && s[i+1] == 0x80 && s[i+2] >= 0x80 && s[i+2] <= 0xBF)
					jump_bytes = 2;
				break;
			}
			case 0xEF: {/*fullwidth ascii and small variants*/
				if ( i+2 < len) {
					switch (s[i+1]){
						case 0xBC: {
							if (s[i+2] >= 0x81 && s[i+2] <= 0xBF)
								sbc_flag = 1;
							break;
						} 
						case 0xBD: {
							if (s[i+2] >= 0x80 && s[i+2] <= 0x9E)
								sbc_flag = 1;
							break;
						}
						case 0xB9: {
							if (s[i+2] >= 0x90 && s[i+2] <= 0xAF)
								jump_bytes = 2;
							break;
						}
						default: break;
					}
				}
				break;
			}
			default : break;
		}
		
		if (sbc_flag == 1) {
			convert.sbc[0] = s[i];
			convert.sbc[1] = s[i+1];
			convert.sbc[2] = s[i+2];
			dbc_sbc_exchange(&convert, 1);
			if (IS_ASCII_PUNC_OR_SYMBOL(convert.dbc)) {/*转换后发现非字母或者数字*/
				jump_bytes = 2;
			}
		}
		
		if (jump_bytes >= 1) {
			i += jump_bytes;
			*m -= (jump_bytes + 1);
		} else {/*ascii and other*/
			if (sbc_flag) {/*转为了半角*/
				if (flag == 0)
					d[j] = xlatcase[convert.dbc];
				else
					d[j] = convert.dbc;
				i += 2;
				*m -= 2;
				j++;
			} else if (IS_ASCII_PUNC_OR_SYMBOL(s[i])) {/*ascii中的非字母或者数字*/
					*m -= 1;//跳过
			} else {
				if (flag == 0)
					d[j] = xlatcase[s[i]];
				else
					d[j] = s[i];
				j++;
			}
		}
	}//for
}

/*
* @brief 将字符串s中的全角转换为半角,多余字节置'-',标点和符号置'+',小写转换为大写,结果串存入d
* @param int len:记录源串s的长度
* @param unsigned char *d: 目的串地址，需调用者提供
* @param unsigned char *s: 源串地址。
* @return void
*/
void string_replace(unsigned char * d, unsigned char *s, int len)
{
	int i,j;
	d_s_utf8_t convert;
	int sbc_flag;
	int jump_bytes;

	for (i = 0, j = 0; i < len; i++)
	{
		sbc_flag = 0;
		jump_bytes = 0;
		switch (s[i]) {
			case 0xC2:{/*latin punctuation 2Bytes*/
				if ( i+1 < len && s[i+1] >= 0xA0 && s[i+1] <= 0xBF) {
					jump_bytes = 1;
				}
				break;
			}
			case 0xC3:{/*multiple and divid 2Bytes*/
				if ( i+1 < len && (s[i+1] == 0x97 || s[i+1] == 0xB7)) {
					jump_bytes = 1;
				}
				break;
			}
			case 0xE2:{/*general punctuation*/
				if ((i+2 < len)
					&& ((s[i+1] == 0x80 && s[i+2] >= 0x80 && s[i+2] <= 0xBF)
						||(s[i+1] == 0x81 && s[i+2] >= 0x80 && s[i+2] <= 0xAF)))
					jump_bytes = 2;
				break;
			}
			case 0xE3: {/*cjk punctuation*/
				if ( i+2 < len && s[i+1] == 0x80 && s[i+2] >= 0x80 && s[i+2] <= 0xBF)
					jump_bytes = 2;
				break;
			}
			case 0xEF: {/*fullwidth ascii and small variants*/
				if ( i+2 < len) {
					switch (s[i+1]){
						case 0xBC: {
							if (s[i+2] >= 0x81 && s[i+2] <= 0xBF)
								sbc_flag = 1;
							break;
						}
						case 0xBD: {
							if (s[i+2] >= 0x80 && s[i+2] <= 0x9E)
								sbc_flag = 1;
							break;
						}
						case 0xB9: {
							if (s[i+2] >= 0x90 && s[i+2] <= 0xAF)
								jump_bytes = 2;
							break;
						}
						default: break;
					}
				}
				break;
			}
			default : break;
		}

		if (sbc_flag == 1) {
			convert.sbc[0] = s[i];
			convert.sbc[1] = s[i+1];
			convert.sbc[2] = s[i+2];
			dbc_sbc_exchange(&convert, 1);
			if (IS_ASCII_PUNC_OR_SYMBOL(convert.dbc)) {/*转换后发现非字母或者数字*/
				jump_bytes = 2;
			}
		}

		if (jump_bytes == 1) {
			d[j] = d[j+1] = '+';
			j += 2;
			i += jump_bytes;
		} else if (jump_bytes == 2) {
			d[j] = d[j+1] = d[j+2] = '+';
			j += 3;
			i+= jump_bytes;
		} else {/*ascii and other*/
			if (sbc_flag) {/*转为了半角*/
				d[j] = xlatcase[convert.dbc];
				d[j+1] = d[j+2] = '-';
				i += 2;
				j += 3;
			} else if (IS_ASCII_PUNC_OR_SYMBOL(s[i])) {/*ascii中的非字母或者数字*/
				d[j] = '+';
				j += 1;
			} else {
				d[j] = xlatcase[s[i]];
				j += 1;
			}
		}
	}//for
}

/**
 * @brief 将源字符串s中一对尖括号中的字符替换成字符'+'
 * 替换过后的内容放置于d中 (最多处理65536个连续开没有闭的括号)
 */
void replace_char_in_angle_brackets(unsigned char *d, unsigned char *s, int len)
{
    int i;
    int j;
    int start;
    int replace_len;
    
    //记录开括号的位置信息
    uint32_t loc_record[65536];
    int total_record = 0;
    memset(loc_record, 0, sizeof(loc_record));

    for (i = 0; i < len; i++) {
        if (s[i] == '<') {
            loc_record[total_record++] = i; //记录开括号位置
            d[i] = s[i];
            if (total_record >= 65536) { // 没法再处理了
                return ;
            }
            continue;            
        }
        if (s[i] != '>') {
            d[i] = s[i];
            continue;
        }
        //遇到闭括号
        if (total_record == 0) { //但是没有开括号
            d[i] = s[i];
            continue;
        }
        
        //遇到闭括号 且有开括号 则最近匹配处理(已经设置过的括号可能会被外层括号的
        //判断重复设置)
        //一个可能的想法是暂不设置遇到的括号,只当记录的开括号pop到只有一个的时候
        //再处理整个最外层的大括号 但假如括号不是一一配对的 就会有问题 比如<abc<d><ddd>
        replace_len = i - loc_record[total_record-1] + 1;
        start = loc_record[total_record-1];
        total_record --;
        for (j = 0; j < replace_len; j++) {
            d[start + j] = '+';
        }
    }
}

