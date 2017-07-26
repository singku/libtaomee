/**
 * @file tm_dirty.h
 * @brief 脏词头文件
 *	Created on:	2011-10-28
 * 	Author:		Singku
 *	Platform:	Linux 2.6.23 kernel x86-32/64
 *	Compiler:	GCC-4.1.2
 *	Copyright:	TaoMee, Inc. ShangHai CN. All Rights Reserved
 */

#ifndef _TM_DIRTY_H
#define _TM_DIRTY_H

#include <sys/ipc.h>
#include <sys/types.h>
#include <stdint.h>
#include <iconv.h>

#define USE_CODE  "UTF-8"

//add by singku 2011-10-26:增加检测消息的最大长度(1M)
#define	MAX_MSG_LEN	(1000000)	

//edit by singku 2011-10-26:改GBK为UTF-8
#define ICONV_START(fromcode) \
	iconv_t cd = iconv_open("UTF-8", fromcode);\
	size_t	inlen;\
	size_t	outlen;\
	char *pin, *pout;

//the inbuf is terminated by '\0'
#define	ICONV_PROC(outbuf,inbuf)\
	do{\
		inlen = strlen(inbuf);\
		outlen = MAX_MSG_LEN;\
		pin = (char *)inbuf;\
		outbuf = (char *)calloc(outlen, 1);\
		pout = outbuf;\
		memset(outbuf, 0, outlen);\
		iconv(cd, &pin, &inlen, &pout, &outlen);\
	}while(0);

#define ICONV_END \
	do{\
		free(outbuf);\
		iconv_close(cd);\
	}while(0);

#define  C_MAX_WORD_LEN   256
#define  C_MAX_TABLE_LEN  256*256
#define  C_MAX_WORD_NUM   3000
#define  E_MAX_WORD_NUM   (C_MAX_WORD_NUM/5)


#define  IS_DOUBLE_CHAR(ch) 		( ch>0x80&&ch<0xFF)
#define  IS_ENGLISH_CHAR(ch)		( (ch>='a'&&ch<='z') || (ch>='A'&&ch<='Z') )
//IS_CHINESE_CHAR这个集合包含了全角英文和全角数字
#define  IS_CHINESE_CHAR(lo,hi)		( (lo>0x80 && lo<0xff) && ((hi>0x3f && hi<0x7f)||(hi>0x7f && hi<0xff)))
//// 中文标点符号
#define  IS_CHINESE_PUNC(lo,hi)		( (lo>0xa0 && lo<0xb0) && (hi>0x3f && hi<0xff))
#define  IS_ENGLISH_DOUBLE(lo,hi)	( (lo==0xA3) &&( (hi<=0xDA && hi>=0xC1) || (hi<=0xFA && hi>=0xE1) ))
#define  IS_DIGIT_DOUBLE(lo,hi) 	( (lo==0xA3) &&(hi>=0xB0 && hi<=0xB9) )
#define  IS_DIGIT(ch)  (isdigit(ch))
//全角??-->大写的
#define  CONVERT_DOUBLE_TO_SINGLE(ch,lo,hi)	ch=(hi>=0xC1&&hi<=0xDA)?(hi-0x80):(hi-0xA0)
#define  CONVERT_DOUBLE_DIGIT_TO_SINGLE(ch,lo,hi)	ch=hi-0x80
#define  CONVERT_CAPITAL_CHAR(ch,lo)	ch=(lo>='a'&&lo<='z')?lo-0x20:lo

#define  EQUAL_ENGLISH_CHAR(lo,hi)  (lo==hi || lo==(hi-0x20))

// 可见的ascii  ' '
#define IS_SHOW_ENG(ch)  (ch>=0x20 && ch<=0x7E)

#define CHAR_KEYWORD_DELI '|'

#define CAL_INDEX_OFFSET(offset,hi,lo)	offset=(hi-0x80)*256+lo;
#define CAL_INDEX_OFFSET2(offset,hi,lo)	offset=(hi-0x80)*256+lo;

#define LOAD_CHAR_FLAG_CHN 'c'
#define LOAD_CHAR_FLAG_ENG 'e'

/*
 * 中文脏词管理结构
 */
typedef struct _DIRTY_CN_RECORD
{
	/*! 过滤级别 (达到这一级别才过滤) */
	int					level;
	/*! 中文脏词 */
	unsigned char		dirty_str[C_MAX_WORD_LEN + 1];
	/*! 脏字眼 */
	unsigned char		key_word[3];
	/*! '脏字眼' 在 dirty_str 中的起始字节偏移 */
	short				key_offset;
	/*! 下一个有相同关键字的脏词记录的索引(in dirty_cn_rec) */
	short				next_key_rec;
} DIRTY_CN_RECORD;

/*
 * 英文脏词管理结构
 */
typedef struct _DIRTY_ENG_RECORD 
{
	/*! 过滤级别 (达到这一级别才过滤) */
	int					level;
	/*! 英文脏词 */
	unsigned char		dirty_str[C_MAX_WORD_LEN + 1];
} DIRTY_ENG_RECORD;

/*
 * 中文'脏字眼'索引管理器
 */
typedef struct _DIRTY_CN_INDEX 
{
	short				dirty_idx_table[C_MAX_TABLE_LEN];
} DIRTY_CN_INDEX;

/*
 * '脏词'管理器
 */
typedef struct _DIRTY_CORE
{
	/*! 内存脏词库是否有效 */
	int					enable;
	/*! 中文脏词记录条数 */
	int					cn_rec_count;
	/*! 英文脏词记录条数 */
	int					en_rec_count;

	/*! 中文'脏字眼'索引管理器 */
	DIRTY_CN_INDEX		dirty_cn_idx_mng;
	/*! 中文脏话纪录 */
	DIRTY_CN_RECORD		dirty_cn_rec[C_MAX_WORD_NUM];
	/*! 英文脏话纪录 */
	DIRTY_ENG_RECORD	dirty_en_rec[E_MAX_WORD_NUM];
} DIRTY_CORE;

/**
 * @typedef tm_dirty_ads_report
 * @brief 广告检测: 提交广告检测数据包的包体结构
 */

/**
 * @struct tm_dirty_ads_report
 * @brief 广告检测：提交广告检测数据包的包体结构
 */
typedef struct tm_dirty_ads_report {
    uint32_t    gameid;
    uint32_t    userid;
    uint32_t    recvid;
    uint32_t    onlineid;
    uint32_t    timestamp;
    uint32_t    msglen;
} __attribute__((packed)) tm_dirty_ads_report_t;
//about ads report

/**
 * @typedef tm_dirty_warning_cb_func_t
 * @brief   短信告警回调函数的类型
 */
typedef void (*tm_dirty_warning_cb_func_t)(const char*, uint32_t, const char*);

extern DIRTY_CN_INDEX		*p_dirty_cn_idx_mng;
extern DIRTY_ENG_RECORD		*p_dirty_en_rec;
extern DIRTY_CORE			*p_dirty_core;

/**
* @brief 去掉 s 头尾的空白符(空格/tab/换行)
 */
int trim_blank(char *s);

/**
 * @brief 脏词检测总入口 return immediately when detecting dirty word
 * @param level: 不使用，兼容接口ACSM_STRUCT
 * @param msg: 待检测的消息
 * @param jump_tag: 是否跳过<>标签 0 否 1是
 * @return		0:	clean;
 * 				1:	has dirty words when just detecting A-Z and a-z; second round
 * 				2:	has dirty words when eliminate A-Z and a-z; third round
 *				3:	has dirty words in original message; first round(混合检测更符合实际情况,所以先检测)
 * 				-1: 出错,dirty handle为空，或者daemon函数未调用,或检测串超长(error_log中有详情).
 */
int tm_dirty_check(int level, char *msg);
int tm_dirty_check_jump_tag(int level, char *msg, int jump_tag);

/**
 * @brief 将msg中的脏词全部替换成字符‘*’
 * @param msg: 待检测的消息
 * @param jump_tag: 是否跳过<>标签 0否 1是
 * @return		0:	clean;没有检测到脏词
 * 				1:	有脏词并得到替换 msg被修改为替换后的msg
 * 				-1: 出错,dirty handle为空，或者daemon函数未调用,或检测串超长.
 */
int tm_dirty_replace(char *msg);
int tm_dirty_replace_jump_tag(char *msg, int jump_tag);
/**
 * @brief 脏词检测总入口 return immediately when detecting dirty word并返回检测到的第一个脏词
 * @param level: 不使用，兼容接口
 * @param msg: 待检测的消息
 * @param retbuf: 检测到的脏词存储缓冲区 要>=256字节长度
 * @param jump_tag: 是否跳过<>标签 0否 1是
 * @return		0:	clean;
 * 				1:	has dirty words when just detecting A-Z and a-z; second round
 * 				2:	has dirty words when eliminate A-Z and a-z; third round
 *				3:	has dirty words in original message; first round(混合检测更符合实际情况,所以先检测)
 * 				-1: 出错,dirty handle为空，或者daemon函数未调用,或检测串超长(error_log中有详情).
 */
int tm_dirty_check_where(int level, char *msg, char *retbuf);
int tm_dirty_check_where_jump_tag(int level, char *msg, char *retbuf, int jump_tag);
/**
 * @brief load脏词 兼容的旧接口.
 * @return -1: 失败, 0: 成功
 */
int tm_load_dirty(const char *conf_file_path);


/**
 * @brief 脏词定期更新函数，首先加载一个脏词，然后开启一个线程进入循环更新过程
 * @param local_dirty_file_path 本地脏词文件路径
 * @param server_addr 脏词数据库服务器地址，包含多个服务器IP和端口 格式"ip1:port1;ip2:port2...."
 * @param update_cycle 更新周期
 * @param warning_function 发送告警短信的回调函数地址.
 * @return -1: 失败, 0: 成功
 */
int
tm_dirty_daemon(
		const char *local_dirty_file_path,
		const char *server_addr,
		uint32_t update_cycle,
        tm_dirty_warning_cb_func_t warning_function);

/**
 * @brief 类似tm_dirty_daemon，只是创建的mmap是有名共享内存，可以被其他平级进程共享
 * @return 如果该函数之前已经被调用过，则直接返回创建过的线程id, 否则返回-1表示失败.
 * @notice 返回值要转换成int判断是否为-1
 */
pthread_t tm_dirty_daemon_with_named_mmap(
        const char *local_dirty_file_path,
		const char *server_addr,
        const char *tm_dirty_share_file_path,
		uint32_t update_cycle,
        tm_dirty_warning_cb_func_t warning_function);

/**
 * @brief print出内存中load到的脏词配置(用UTF-8终端查看)
 * @param char_set: 不使用，兼容接口
 * @return 0成功，-1失败
 */
int tm_dirty_list_word(int char_set);

/**
 * @brief 返回脏词数
 * @return 脏词数 -1失败
 */
int tm_dirty_word_count(void);

/**
 * @brief 释放脏词 dirty_handled对应的内存
 */
int tm_dirty_free();

/**
 *
 */
int attach_to_dirty_shm(char* tm_dirty_file);

//=================about ads report added by singku 2011-11-08====================
/*
 * @brief init UDP socket for ads report
 * @param ads_report_udp_ip: ads server's ip
 * @param ads_report_udp_port: ads server's port
 * @return 0: success, -1 failed
 */
int init_ads_report_udp_socket(const char *ads_report_udp_ip, uint16_t ads_report_udp_port);
    

/**
 * @brief send a UDP ads report to db
 * @param ads_info_head: detailed msg about an ads
 * @param msg: actual msg
 * @return 0 on success, -1 on error
 */
int send_udp_ads_report_to_db(tm_dirty_ads_report_t *ads_info_head, const char *msg);

//=====================================


#endif  /* _TM_DIRTY_H_ */
