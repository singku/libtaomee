/*
 * tm_dirty.c
 *
 *	Created on:	2011-10-28
 * 	Author:		Singku
 *	Platform:	Linux 2.6.23 kernel x86-32/64
 *	Compiler:	GCC-4.1.2
 *	Copyright:	TaoMee, Inc. ShangHai CN. All Rights Reserved
 */

//needed for getline and io operation
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>

//needed for free
#include <stdlib.h>

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>

//needed for load dirty from server
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <errno.h>

//needed for generate file's md5
#include <openssl/md5.h>

//needed for update thread
#include <pthread.h>

//needed for dir create
#include <glib.h>
#include <libgen.h>

#include <libtaomee/inet/tcp.h>
#include <libtaomee/inet/inet_utils.h>
#include <libtaomee/log.h>
#include <libtaomee/conf_parser/config.h>

#include "utf8_punc.h"
#include "acsm.h"
#include "tm_dirty.h"

/*----start------want to be invisible by user who will include tm_dirty.h */

#define	DIRTY_FILE_PACKET_FIX_LEN (sizeof(tm_dirty_protocol_t) + sizeof(tm_dirty_ret_packet_get_dirty_t) + sizeof(uint32_t))
#define	DIRTY_FILE_PACKET_MAX_LEN (DIRTY_FILE_PACKET_FIX_LEN + 4096)

/**
 * @enum tm_dirty_proto_cmd
 * @brief protocol definition of communicating with dirty server
 * @brief 65001:请求服务器的脏词文件md5; 65002请求脏词文件; 65003:提交广告检测.
 */
enum tm_dirty_proto_cmd {
	proto_dirty_cmp_md5					= 65001,
	proto_dirty_get_file				= 65002,
    proto_dirty_ads_report              = 65003,
};

/**
 * @typedef tm_dirty_protocol
 * @brief 脏词更新：交互协议头
 */

/**
 * @struct tm_dirty_protocol
 * @brief protocol definition of packet head in application layer
 */
typedef struct tm_dirty_protocol {
	uint32_t	len;
	uint32_t	seq;
	uint16_t	cmd;
	uint32_t	ret;
	uint32_t	id;
} __attribute__((packed)) tm_dirty_protocol_t;

/**
 * @typedef tm_dirty_ret_packet_cmp_md5 
 * @brief 脏词更新：服务器回MD5包的包体结构
 */

/**
 * @struct tm_dirty_ret_packet_cmp_md5
 * @brief protocol definition of packet body, got from server with md5
 */
typedef struct tm_dirty_ret_packet_cmp_md5 {
	uint8_t		md5[16];
} __attribute__((packed)) tm_dirty_ret_packet_cmp_md5_t;

/**
 * @typedef tm_dirty_packet_get_dirty
 * @brief 脏词更新：客户端发送脏词文件请求的包体结构
 */

/**
 *@struct tm_dirty_packet_get_dirty
 * @brief  protocol definition of packet body, send file request from client with own md5
 */
typedef struct tm_dirty_packet_get_dirty {
	uint8_t		md5[16];
	uint8_t		reverse[32];
} __attribute__((packed)) tm_dirty_packet_get_dirty_t;

/**
 * @typedef tm_dirty_ret_packet_get_dirty
 * @brief 脏词更新：服务器回送脏词文件的包体结构
 */

/**
 * @typedef tm_dirty_ret_packet_get_dirty
 * @brief protocol definition of packet body(fragment), got from server with file block
 */
typedef struct tm_dirty_ret_packet_get_dirty {
	uint8_t		md5[16];
	uint32_t	totallen;
	uint32_t	endable;
	uint8_t		reverse[32];
	//uint32_t file block length; var_len 4096
	//uint8_t file block[];
} __attribute__((packed)) tm_dirty_ret_packet_get_dirty_t;

/*----end------want to be invisible by user who will include tm_dirty.h */

//dirty handle, used alternately when updating 0 1用由于新接口，2用于老接口
ACSM_STRUCT *tm_dirty_handle[2] = {NULL, NULL};
ACSM_STRUCT *tm_dirty_inner_handle[2] = {NULL, NULL};
//dirty server ip, main ip and back up ip, initiated by default value, determined by tm_dirty_daemon's parameters
char tm_dirty_server_ip[16][16] = {"192.168.4.68", "10.1.1.155", "10.1.1.46"};
uint16_t tm_dirty_server_port[16] = {28000, 28000, 28000};
uint16_t tm_dirty_valid_server_count = 3;
uint16_t tm_dirty_available_server_flag = 0;//记录可用的IP标记，下次直接从这个IP开始循环查找.
char tm_dirty_local_dirty_file_path[1024] = "./data/dirty.dat";
uint32_t tm_dirty_update_cycle = 600;//更新周期 seconds

uint8_t local_md5_dgst[16] = {0};			//本地md5
uint8_t local_md5_exist = 0;					//本地md5是否已经存在标记,本函数会被多次调用
uint8_t servr_md5_dgst[16] = {0};			//服务器md5
uint8_t servr_md5_recorded = 0;				//是否已记录服务器md5

int daemon_called = 0;	//是否调用了daemon函数
pthread_t loop_thread = 0;//保存更新线程的线程ID
tm_dirty_warning_cb_func_t tm_dirty_warning_func = NULL;

//about ads report
struct sockaddr_in ads_report_udp_sa;
int ads_report_udp_fd = -1;

static int tm_dirty_load_new(const char *conf_file_path);

/**
 * @brief 协议头初始化
 * @param buf:协议数据缓冲区, len: 协议数据长度, cmd:协议号, id:0, seq:0, ret:0
 * @return void
 */
static inline void
tm_dirty_init_proto_head(void* buf, int len, int cmd, uint32_t id, int seq, int ret)
{
#ifdef __cplusplus
	tm_dirty_protocol_t* p = reinterpret_cast<protocol_t*>(buf);
#else
	tm_dirty_protocol_t* p = buf;
#endif
	p->len = (len);
	p->cmd = (cmd);
	p->id  = (id);
	p->seq = (seq);
	p->ret = (ret);
}

//=================about ads report added by singku 2011-11-08====================
/**
 * @brief init UDP socket for ads report
 * @param ads_report_udp_ip: ads server's ip
 * @param ads_report_udp_port: ads servers's port
 * @return 0: success, -1: failed
 */
int init_ads_report_udp_socket(const char *ads_report_udp_ip, uint16_t ads_report_udp_port)
{
    bzero(&ads_report_udp_sa, sizeof(ads_report_udp_sa));
    ads_report_udp_sa.sin_family = AF_INET;
    ads_report_udp_sa.sin_port = htons(ads_report_udp_port);
    if (inet_pton(AF_INET, ads_report_udp_ip, &(ads_report_udp_sa.sin_addr)) <= 0)
        return -1;
    return (ads_report_udp_fd = socket(PF_INET, SOCK_DGRAM, 0));
}


/**
 * @brief send a UDP ads report to db
 * @param ads_info_head: detailed msg about an ads
 * @param msg: actual msg
 * @return 0 on success, -1 on error
 */
int send_udp_ads_report_to_db(tm_dirty_ads_report_t *ads_info_head, const char *msg)
{
    static uint8_t dbbuf[MAX_MSG_LEN];
    static uint32_t error_log_count = 0;

    if (ads_report_udp_fd == -1) {
        if (error_log_count < 100) {
            ERROR_RETURN(("tm_dirty: [%03u]\t send ads report to udp db failed: fd=%d", ++error_log_count, ads_report_udp_fd), -1);
        } else {
            return -1;
        }
    }

    uint32_t len = sizeof(tm_dirty_protocol_t) + sizeof(tm_dirty_ads_report_t) + ads_info_head->msglen;
    
    if (len > (sizeof(dbbuf))) {
        KERROR_LOG(0, "tm_dirty: send talk report to udp db failed: fd=%d len=%d", ads_report_udp_fd, len);
        return -1;
    }

    tm_dirty_init_proto_head(dbbuf, len, proto_dirty_ads_report, ads_info_head->userid, 0, 0);

    memcpy(dbbuf + sizeof(tm_dirty_protocol_t), ads_info_head, sizeof(tm_dirty_ads_report_t));
    memcpy(dbbuf + sizeof(tm_dirty_protocol_t) + sizeof(tm_dirty_ads_report_t), msg, ads_info_head->msglen);

    return sendto(ads_report_udp_fd, dbbuf, len, 0, (const struct sockaddr*)(&ads_report_udp_sa), sizeof(ads_report_udp_sa));
}

//=====================================


/**
 *@brief 生成文件的md5值
 * @param file_path: 文件路径, dgst:存放md5的缓冲区
 * @return 0:成功，-1:文件不存在或者md5函数调用失败.
 */
int tm_dirty_generate_md5_from_file(const char *file_path, uint8_t *dgst)
{

	FILE *fp = fopen(file_path, "rb");
	if (fp == NULL)
		return -1;

	int md5_ret;
	MD5_CTX md5_handle;
	if ((md5_ret = MD5_Init(&md5_handle)) == 0)
		return -1;

	unsigned char buf[1024];
	int read_len;

	while((read_len = fread(buf, 1, sizeof(buf), fp)) > 0) {
		md5_ret = MD5_Update(&md5_handle, buf, read_len);
    	if (md5_ret == 0)
    		return -1;
    }
	fclose(fp);

	if ((md5_ret = MD5_Final(dgst, &md5_handle)) == 0)
		return -1;

	return 0;
}

/**
 * @brief 从服务器加载脏词文件
 * @param void
 * @return 0:成功地从服务器加载新脏词文件，1:本地脏词文件存在，且与服务器脏词一致，-1:加载失败.
 */
int tm_dirty_load_from_server(void)
{
	uint8_t buf[8192];				//收发缓冲区
	uint8_t *buf_ptr;				//缓冲区指针
	int fd;							//连接上的服务器的socket fd

	int total_send_len;				//总的发送字节数
	int cur_send_len;				//本次send发送字节数
	int total_recv_len;				//总的收取字节数
	int cur_recv_len;				//本次recv的字节数
	int proto_head_len;				//协议头长度

	tm_dirty_ret_packet_get_dirty_t *packet_body;
	tm_dirty_protocol_t *proto_head;

	int packet_len;					//数据包长度
	int rest_len;					//剩余长度，一次recv没有收完的数据包的遗留长度
	int needed_len;					//还需要多长才能达到一个完成的包头
	int over_read_bytes;			//read length longer than this packet len;	//一次recv大于一个协议包的长度
	int file_len;					//脏词文件长度
	char new_file[1024];			//如果本地文件存在，则新建一个文件.new
	FILE *fp = NULL;
	int file_closed;				//新文件是否已经关闭，有可能传输中断导致文件不完整 没有关闭
	int load_file_ok = 1;			//新文件是否已经完全加载


	servr_md5_recorded = 0;

    //create dir if dir not exist
    char dirpath[1024];
    memcpy(dirpath, tm_dirty_local_dirty_file_path, sizeof(dirpath));
    if (g_mkdir_with_parents(dirname(dirpath), 0755) == -1) {
        ERROR_RETURN(("tm_dirty: failed to create dir: %s", dirpath), -1);
    }

	//check local file's existence and set md5 key
	if (access(tm_dirty_local_dirty_file_path, F_OK) == -1) {
		if (errno == ENOENT) {//本地文件不存在，则本地md5直接置0
			memset(local_md5_dgst, 0x0, sizeof(local_md5_dgst));
		} else {//本地文件存在
			if (!local_md5_exist) {//但本地md5不存在(第一次调用此函数)，则生成一个
				if (tm_dirty_generate_md5_from_file(tm_dirty_local_dirty_file_path, local_md5_dgst) == 0) {
					local_md5_exist = 1;
				} else {//md5生成失败，返回-1
					ERROR_RETURN(("tm_dirty: generate md5 failed"), -1);
				}
			}
		}
	} else {	//本地文件存在
		if (!local_md5_exist) {
			if (tm_dirty_generate_md5_from_file(tm_dirty_local_dirty_file_path, local_md5_dgst) == 0) {
				local_md5_exist = 1;
			} else {
				ERROR_RETURN(("tm_dirty: generate md5 failed"), -1);
			}
		}
	}

	total_send_len = 0;
	cur_send_len = 0;
	total_recv_len = 0;
	cur_recv_len = 0;
	proto_head_len = sizeof(tm_dirty_protocol_t);

	//choose server
    int i;
    for (i = 0; i < tm_dirty_valid_server_count; i++){
        int flag = (i + tm_dirty_available_server_flag) % tm_dirty_valid_server_count;
        if ((fd = safe_tcp_connect(tm_dirty_server_ip[flag], tm_dirty_server_port[flag], 2, 0)) != -1){
            tm_dirty_available_server_flag = flag;
            break;    
        }
    }

    static int connect_failed_counter = 0;
    if (fd == -1) {
        if (tm_dirty_warning_func != NULL && connect_failed_counter++ == 3) {
            char ip[16];
            connect_failed_counter = 0;
            if (get_ip_addr("eth1", AF_INET, ip, 16) == 0 
                || get_ip_addr("eth0", AF_INET, ip, 16) == 0) {
                tm_dirty_warning_func("tm_dirty: none update server available.", 0, ip);
            } else {
                tm_dirty_warning_func("tm_dirty: none update server available.", 0, 0);
            }
        }
        ERROR_RETURN(("tm_dirty: connect to dirty server failed"), -1);
    } else {
        connect_failed_counter = 0;
    }
    //设置套接字的收发时间限制，以免服务器连接上后 收包丢失导致卡死2000ms
    
    if (set_sock_snd_timeo(fd, 2000) == -1)
        ERROR_LOG("set dirty socket send timeout failed");
    if (set_sock_rcv_timeo(fd, 2000) == -1)
        ERROR_LOG("set dirty socket recv timeout failed");
    
    
	//cmp md5 with server
	total_send_len = sizeof(tm_dirty_protocol_t);
	tm_dirty_init_proto_head(buf, total_send_len, proto_dirty_cmp_md5, 0, 0, 0);
	if (safe_tcp_send_n(fd, buf, total_send_len) < total_send_len) {
		close(fd);
        ERROR_RETURN(("tm_dirty: send md5 request: send too short bytes"), -1);
    }

	total_recv_len = sizeof(tm_dirty_protocol_t) + sizeof(servr_md5_dgst);
    int n;
	if ((n = safe_tcp_recv_n(fd, buf, total_recv_len)) < total_recv_len) {
		close(fd);
        ERROR_RETURN(("tm_dirty: recv server md5: get too short bytes"), -1);
	}

	proto_head = (tm_dirty_protocol_t*)buf;
	if (proto_head->cmd != proto_dirty_cmp_md5 || proto_head->len != total_recv_len) {
        close(fd);
		ERROR_RETURN(("tm_dirty: recv server md5: invalid packet\t[cmd=%d;len=%d]", proto_head->cmd, proto_head->len), -1);
    }

	buf_ptr = buf + sizeof(tm_dirty_protocol_t);

	if (memcmp(local_md5_dgst, buf_ptr, 16) == 0) {
		close(fd);
		return -1;	//本地md5与server md5匹配 无须更新 若为第一次加载，则返回-1告知daemon函数，从本地载入
	}

	//get dirty from server
	total_send_len = sizeof(tm_dirty_protocol_t) + sizeof(tm_dirty_packet_get_dirty_t);
	tm_dirty_init_proto_head(buf, total_send_len, proto_dirty_get_file, 0, 0, 0);
	if (safe_tcp_send_n(fd, buf, total_send_len) < total_send_len) {
		close(fd);
        ERROR_RETURN(("tm_dirty: send 'file request': send too short bytes"), -1);
	}

	total_recv_len = 0;
	file_len = 0;
	fp = NULL;
	file_closed = 1;//新创建的脏词文件(.new)是否已经关闭，初始状态未打开，故1表示关闭.

	cur_recv_len = 0;
	needed_len = proto_head_len - cur_recv_len;
	buf_ptr = buf;
	load_file_ok = 0;
	while ( (cur_recv_len >= proto_head_len)
			|| ((cur_recv_len += safe_tcp_recv_n(fd, buf_ptr, needed_len)) >= needed_len)) {
		total_recv_len = cur_recv_len;
		buf_ptr = buf;
		proto_head = (tm_dirty_protocol_t*)buf_ptr;
		packet_len = proto_head->len;

		if (proto_head->cmd != proto_dirty_get_file || proto_head->len > DIRTY_FILE_PACKET_MAX_LEN) {
            close(fd);
            if (fp != NULL) {
                fclose(fp);
            }
			ERROR_RETURN(("tm_dirty: recv dirty file: invalid packet\t[cmd=%d;len=%d]", proto_head->cmd, proto_head->len), -1);
        }

		buf_ptr = buf + total_recv_len;
		if (cur_recv_len < packet_len) {
			rest_len = packet_len - cur_recv_len;
			cur_recv_len = safe_tcp_recv_n(fd, buf_ptr, rest_len);
            
            if (cur_recv_len == rest_len)
                goto recvok;

            while (cur_recv_len < rest_len && cur_recv_len > 0) {
                ERROR_LOG("tm_dirty: recving_error:%s, EAGAIN",strerror(errno));
                total_recv_len += cur_recv_len;
                rest_len = rest_len - cur_recv_len;
                buf_ptr += cur_recv_len;
                cur_recv_len = safe_tcp_recv_n(fd, buf_ptr, rest_len);
                if (cur_recv_len == rest_len)
                    goto recvok;
            }

            ERROR_LOG("tm_dirty: recving_error:%s", strerror(errno));
            close(fd);
            if (fp != NULL)	{//第一次断开，新文件还未被打开依然是NULL 如果中途断开，则文件被打开 fp不为NULL
                fclose(fp);
            }
            ERROR_RETURN(("tm_dirty: recv dirty file block: incomplete packet!"), -1);
recvok:			
            total_recv_len += cur_recv_len;
		}

		packet_body = (tm_dirty_ret_packet_get_dirty_t*)(buf + proto_head_len);

		if ((packet_body->endable != 0 && packet_body->endable != 1)) {
            close(fd);
            if (fp != NULL) {
                fclose(fp);
            }
		    ERROR_RETURN(("tm_dirty: recv dirty file,invalid packet[endable=%d(MUST BE 0 or 1)]", packet_body->endable),-1);    
        }

		//same md5, then file_len will be set to zero
		if (packet_body->totallen == 0) {
			close(fd);
			if (fp != NULL) {
				fclose(fp);
            }
			return 0;//no need to update file
		}

		if (file_len != 0) {
			if (file_len != packet_body->totallen) {
				close(fd);
				if (fp != NULL) {
					fclose(fp);
                }
				ERROR_RETURN(("tm_dirty: dirty file's length doesn't match with the 'totallen' in the packet\n"), -1);
			}
		} else {
			file_len = packet_body->totallen;
		}

		//record server's md5;
		if (!servr_md5_recorded) {
			memcpy(servr_md5_dgst, packet_body->md5, sizeof(servr_md5_dgst));
			servr_md5_recorded = 1;
		}

		buf_ptr = buf + sizeof(tm_dirty_protocol_t) + sizeof(tm_dirty_ret_packet_get_dirty_t);
		uint32_t block_len = *((uint32_t*)buf_ptr);
		
		if (proto_head->len != (DIRTY_FILE_PACKET_FIX_LEN + block_len)) {
            close(fd);
            if (fp != NULL) {
                fclose(fp);
            }
			ERROR_RETURN(("tm_dirty: recv dirty file, invalid packet\t[annonunced_len:%u, actual_len:%lu]",
					proto_head->len, (DIRTY_FILE_PACKET_FIX_LEN + block_len)), -1);
        }
		
		buf_ptr = buf_ptr + sizeof(uint32_t);
		//write block 2 file
		if (fp == NULL) {
			snprintf(new_file, sizeof(new_file), "%s.new", tm_dirty_local_dirty_file_path);
			fp = fopen(new_file, "wb");
			if (fp == NULL) {
				close(fd);
				ERROR_RETURN(("tm_dirty: can't create new dirty file[%s,%s]", new_file, strerror(errno)), -1);
			}
			file_closed = 0;
		}
		fwrite(buf_ptr, 1, block_len, fp);

		if (packet_body->endable == 1) { //no more file blocks transfered
			close(fd);
			fclose(fp);
			load_file_ok = 1;
			file_closed = 1;
            break;
		}

		buf_ptr += block_len;
		//move over read bytes back to buf's start addr from buf_ptr
		over_read_bytes = total_recv_len - packet_len;
		if (over_read_bytes > 0) {
			memcpy(buf, buf_ptr, over_read_bytes);
		}

		//reset next packets value;
		cur_recv_len = over_read_bytes;
		needed_len = proto_head_len - over_read_bytes;
		buf_ptr = buf + cur_recv_len;
	}//while file

	if (!load_file_ok) {
		close(fd);
		if (!file_closed) {
			fclose(fp);
        }
		ERROR_RETURN(("tm_dirty: update dirty file failed"), -1);
	}

	if (tm_dirty_generate_md5_from_file(new_file, local_md5_dgst) == -1)
		ERROR_RETURN(("tm_dirty: diryt file updated successfully, but generate new md5 failed"), -1);

	if (memcmp(local_md5_dgst, servr_md5_dgst, 16) != 0)
		ERROR_RETURN(("tm_dirty: new dirty file's md5 doesn't match with sever's md5\n"), -1);

    remove(tm_dirty_local_dirty_file_path);
    rename(new_file, tm_dirty_local_dirty_file_path);//更名

	//load dirty from new file
	if (tm_dirty_load_new(tm_dirty_local_dirty_file_path) == -1)
		ERROR_RETURN(("tm_dirty: dirty file updated, but failed when load to memory"), -1);
    
    DEBUG_LOG("tm_dirty: dirty file updated and load to mem successfully");
	return 0;
}

/**
 * @brief 脏词更新的线程函数.
 * @param void
 * @return void
 */
void tm_dirty_update_thread(void)
{
	while (1) {
		sleep(tm_dirty_update_cycle);
		tm_dirty_load_from_server();
	}
}

/**
 * @brief 脏词定期更新函数，首先加载一个脏词，然后开启一个线程进入循环更新过程
 * @param local_dirty_file_path 本地脏词文件路径
 * @param server_addr 脏词数据库服务器地址，包含多个IP和端口的字符串。格式"ip1:port1;ip2:port2...."
 * @param update_cycle 更新周期
 * @param warning_function 告警回调函数的地址.
 * @return -1: 失败, 0: 成功
 */
int
tm_dirty_daemon(
		const char *local_dirty_file_path,
		const char *server_addr,
		uint32_t update_cycle,
        tm_dirty_warning_cb_func_t warning_function)
{
	//防止重复调用.父进程调用后该值为1, 继承给子进程后，子进程中也为1,也就调用无效了.
	if (daemon_called)
		return 0;
		
	//set global variable with parameters
	int len, i;

    if (local_dirty_file_path != NULL) {
        len = strlen(local_dirty_file_path) + 1;
        len = (len > sizeof(tm_dirty_local_dirty_file_path)) ?sizeof(tm_dirty_local_dirty_file_path) :len;
        memcpy(tm_dirty_local_dirty_file_path, local_dirty_file_path, len);
    }

    char buf[1024];
    if (server_addr != NULL) {
        char *colon;//冒号
        char *semicolon;//分号
        char *str_ptr = (char *)server_addr;
        i = 0;
        while(*str_ptr != '\0' && i < sizeof(buf)) {
            if ((*str_ptr >= '0' && *str_ptr <= '9') || *str_ptr == ':' || *str_ptr == ';' || *str_ptr == '.')
                buf[i++] = *str_ptr;
            str_ptr++;
        }
        buf[i] = 0;
        i = 0;
        str_ptr = buf;
        while ((colon = strchr(str_ptr, (int)(':'))) != NULL){
            len = colon - str_ptr;
            
            if (len > 15) {//xxx.xxx.xxx.xxx
                ERROR_RETURN(("tm_dirty: server's ipv4 addr is too long [%d], max=15bytes", len), -1);
            }
            
            memcpy(tm_dirty_server_ip[i], str_ptr, len);
            tm_dirty_server_ip[i][len] = 0;

            if (*(colon + 1) != '\0')
                tm_dirty_server_port[i] = (uint16_t)strtoul(colon + 1, NULL, 10);
            
            if (is_legal_ip(tm_dirty_server_ip[i]) && is_legal_port(tm_dirty_server_port[i])) {
                i++;
            } else {
                ERROR_RETURN(("tm_dirty: invalid server ipv4 addr[%s:%d]", tm_dirty_server_ip[i], tm_dirty_server_port[i]), -1);
            }

            if ((semicolon = strchr((colon + 1), (int)(';'))) == NULL) {
                break;
            } else {
                str_ptr = semicolon + 1;
            }
        }
        if (i == 0){
            ERROR_RETURN(("tm_dirty: no valid tm_dirty server found. ex: 192.168.0.1:28000;192.168.0.2;28001"), -1);
        }
        tm_dirty_valid_server_count = i;
    } else {
        ERROR_RETURN(("tm_dirty: dirty server's configuration not found!"), -1);
    }

    if (update_cycle >= 10) {
        tm_dirty_update_cycle = update_cycle;
    }

    if (warning_function != NULL) {
        tm_dirty_warning_func = warning_function;
    }
    
    int putn = 0;
    for (i = 0 ; i < tm_dirty_valid_server_count && i < 16; i++) {
        putn += snprintf(buf + putn, sizeof(buf), "\n\taddr[%02d]\t\t%-15s:%-5u", 
                i, tm_dirty_server_ip[i], tm_dirty_server_port[i]);
    }
    snprintf(buf + putn, sizeof(buf) - putn, "\n\tupdate_cycle\t\t%u\n\tdirty_file\t\t%s\n\twarning_cb_func_addr\t%p", 
            tm_dirty_update_cycle, tm_dirty_local_dirty_file_path, tm_dirty_warning_func);
    DEBUG_LOG("TM_DIRTY:%s", buf);
    
	tm_dirty_handle[0] = acsmNew();
	tm_dirty_handle[1] = acsmNew();
    tm_dirty_inner_handle[0] = tm_dirty_handle[0];
    tm_dirty_inner_handle[1] = tm_dirty_handle[1];

	if (tm_dirty_handle[0] == NULL || tm_dirty_handle[1] == NULL)
		ERROR_RETURN(("tm_dirty: get two mmap mem block failed!"), -1);

	tm_dirty_handle[0]->flag = 0;
	tm_dirty_handle[1]->flag = 0;

	//load dirty first handle;
	int ret;
    if ((ret = tm_dirty_load_from_server()) == -1) {
        if (local_md5_exist) {//load dirty from existed file
            if (tm_dirty_load_new(tm_dirty_local_dirty_file_path) == -1) {
                ERROR_RETURN(("tm_dirty: first load,servers down but local dirty file exists, failed when load to memory"), -1);
            } else {
                DEBUG_LOG("tm_dirty: first load,load from local dirty file successfully");
            }
        } else {
            ERROR_RETURN(("tm_dirty: first load, servers unreachable and no local dirty file "), -1);
        }

    }

	daemon_called = 1;
	//create thread diry_loop
	if(pthread_create(&loop_thread, NULL, (void*(*)(void*))tm_dirty_update_thread, NULL) != 0)
		ERROR_RETURN(("can not create thread:dirty_update_thread\t"), -1);

	return 0;
}

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
        tm_dirty_warning_cb_func_t warning_function)
{
	//防止重复调用.父进程调用后该值为1, 继承给子进程后，子进程中也为1,也就调用无效了.
	if (daemon_called)
		return loop_thread;

	//set global variable with parameters
	int len, i;

	if (local_dirty_file_path != NULL) {
		len = strlen(local_dirty_file_path) + 1;
		len = (len > sizeof(tm_dirty_local_dirty_file_path)) ?sizeof(tm_dirty_local_dirty_file_path) :len;
		memcpy(tm_dirty_local_dirty_file_path, local_dirty_file_path, len);
	}

	char buf[1024];
	if (server_addr != NULL) {
		char *colon;//冒号
		char *semicolon;//分号
		char *str_ptr = (char *)server_addr;
		i = 0;
		while(*str_ptr != '\0' && i < sizeof(buf)) {
			if ((*str_ptr >= '0' && *str_ptr <= '9') || *str_ptr == ':' || *str_ptr == ';' || *str_ptr == '.')
				buf[i++] = *str_ptr;
			str_ptr++;
		}
		buf[i] = 0;
		i = 0;
		str_ptr = buf;
		while ((colon = strchr(str_ptr, (int)(':'))) != NULL){
			len = colon - str_ptr;

			if (len > 15) {//xxx.xxx.xxx.xxx
				ERROR_RETURN(("tm_dirty: server's ipv4 addr is too long [%d], max=15bytes", len), -1);
			}

			memcpy(tm_dirty_server_ip[i], str_ptr, len);
			tm_dirty_server_ip[i][len] = 0;

			if (*(colon + 1) != '\0')
				tm_dirty_server_port[i] = (uint16_t)strtoul(colon + 1, NULL, 10);

			if (is_legal_ip(tm_dirty_server_ip[i]) && is_legal_port(tm_dirty_server_port[i])) {
				i++;
			} else {
				ERROR_RETURN(("tm_dirty: invalid server ipv4 addr[%s:%d]", tm_dirty_server_ip[i], tm_dirty_server_port[i]), -1);
			}

			if ((semicolon = strchr((colon + 1), (int)(';'))) == NULL) {
				break;
            } else {
                str_ptr = semicolon + 1;
            }
        }
        if (i == 0){
            ERROR_RETURN(("tm_dirty: no valid tm_dirty server found. ex: 192.168.0.1:28000;192.168.0.2;28001"), -1);
        }
        tm_dirty_valid_server_count = i;
    } else {
        ERROR_RETURN(("tm_dirty: dirty server's configuration not found!"), -1);
    }

    if (update_cycle >= 10) {
        tm_dirty_update_cycle = update_cycle;
    }

    int putn = 0;
    for (i = 0 ; i < tm_dirty_valid_server_count && i < 16; i++) {
        putn += snprintf(buf + putn, sizeof(buf), "\n\taddr[%02d]\t\t%-15s:%-5u",
                i, tm_dirty_server_ip[i], tm_dirty_server_port[i]);
    }
    snprintf(buf + putn, sizeof(buf) - putn, "\n\tupdate_cycle\t\t%u\n\tdirty_file\t\t%s\n\t",
		    tm_dirty_update_cycle, tm_dirty_local_dirty_file_path);
    DEBUG_LOG("TM_DIRTY:%s", buf);

    int fd = open(tm_dirty_share_file_path, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
	    ERROR_RETURN(("open dirty share file failed:%s", strerror(errno)), -1);
    }

	acsmNew_with_fd(fd, &(tm_dirty_handle[0]), &(tm_dirty_handle[1]));
    tm_dirty_inner_handle[0] = tm_dirty_handle[0];
    tm_dirty_inner_handle[1] = tm_dirty_handle[1];

	if (tm_dirty_handle[0] == NULL || tm_dirty_handle[1] == NULL) {
		ERROR_RETURN(("tm_dirty: get two mmap mem block failed!"), -1);
    }

	tm_dirty_handle[0]->flag = 0;
	tm_dirty_handle[1]->flag = 0;
  

	//load dirty first handle;
	int ret;
    if ((ret = tm_dirty_load_from_server()) == -1) {
        if (local_md5_exist) {//load dirty from existed file
            if (tm_dirty_load_new(tm_dirty_local_dirty_file_path) == -1) {
                ERROR_RETURN(("tm_dirty: first load,servers down but"
                            " local dirty file exists, failed when load to memory"), -1);
            } else {
                DEBUG_LOG("tm_dirty: first load,load from local dirty file successfully");
            }
        }
        else
        {
            ERROR_RETURN(("tm_dirty: first load, servers unreachable and no local dirty file "), -1);
        }

    }

	daemon_called = 1;
	//tm_dirty_update_thread();//更新脏词
#if 1
    if(pthread_create(&loop_thread, NULL, (void*(*)(void*))tm_dirty_update_thread, NULL) != 0)
		ERROR_RETURN(("can not create thread:dirty_update_thread\t"), -1);
#endif
    return loop_thread;
}


/**
 * @brief load脏词 新接口 内部使用
 * @return -1: 失败, 0: 成功
 */
static int tm_dirty_load_new(const char *conf_file_path)
{

	int update_flag = tm_dirty_handle[0]->flag ^ 1;//待更新的dirty_handle标记，不能是当前使用的handle

	tm_dirty_handle[update_flag]->acsmMaxStates = 0;
	tm_dirty_handle[update_flag]->acsmNumStates = 0;
	tm_dirty_handle[update_flag]->total_pattern = 0;

	char *dirty_word = NULL;
	char identify_char;
	size_t len = 0;
	size_t read;

	FILE *fp = fopen(conf_file_path, "rt");
	if (fp == NULL) {
		ERROR_RETURN(("CANT OPEN DIRTY FILE!"), -1);
	}
	char *tab;
	int dirty_len;
	while (!feof(fp)) {
		identify_char = fgetc(fp);
		if ((int)identify_char == -1)
			break;
		if (identify_char == '#') {
			while (identify_char != '\n' && !feof(fp))
				identify_char = fgetc(fp);
			continue;
		} else if (identify_char == ' ' || identify_char == '\t') {
			while(identify_char == ' ' || identify_char == '\t')
				identify_char = fgetc(fp);
			fseek (fp, -1L, SEEK_CUR);
			continue;
		} else if (identify_char == '\n')
			continue;

		fseek (fp, -1L, SEEK_CUR);
		if ((read = getline(&dirty_word, &len, fp)) == -1)
			break;
		tab = strchr(dirty_word, '\t');
		if (tab != NULL) {
			dirty_len = tab - dirty_word;
		} else {
			dirty_len = strlen(dirty_word) -1;
		}

        if (dirty_len == 0)
            continue;

		if (acsmAddPattern(tm_dirty_handle[update_flag], (unsigned char *)dirty_word, dirty_len) == -1) {
			ERROR_LOG("dirty patterns reached MAX_PATTERN, the rest patterns will be ignored");
			break;
		}
	}
	fclose(fp);

	if (acsmCompile(tm_dirty_handle[update_flag]) == -1)
		ERROR_RETURN(("compile dirty handle failed"), -1);

	if(dirty_word)
		free(dirty_word);

	//切换匹配树
	tm_dirty_handle[0]->flag = tm_dirty_handle[1]->flag = update_flag;
	//tm_dirty_list_word(1);

	return 0;
}

/**
 * @brief load脏词 旧接口，供老代码外部使用
 * @brief 必须加锁.
 * @return -1: 失败, 0: 成功
 */
int tm_load_dirty(const char *conf_file_path)
{
	if (daemon_called)	{//如果已经调用了daemon函数 则此接口无效
		return 0;
	} else {
        /*
		BOOT_LOG(-1, "reason 1: needed to update asyncserver to 1.6.1;\n\
                reason 2: old interface [tm_load_dirty] called, \
                but variable 'tm_dirty_use_dirty_logical is set to 0'");
        */
		return 0;
	}
}

static int dirty_err_count = 0;
static inline int common_routine(int *flag)
{
	if (likely(daemon_called)) {
        if (unlikely(tm_dirty_handle[0]->flag != tm_dirty_inner_handle[0]->flag)) {
            tm_dirty_handle[0]->flag = tm_dirty_inner_handle[0]->flag;
            tm_dirty_handle[0]->acsmMaxStates = tm_dirty_inner_handle[0]->acsmMaxStates;
            tm_dirty_handle[0]->acsmNumStates = tm_dirty_inner_handle[0]->acsmNumStates;
            tm_dirty_handle[0]->total_pattern = tm_dirty_inner_handle[0]->total_pattern;
        }
		*flag = tm_dirty_handle[0]->flag;
	} else {
        char *path = config_get_strval("tm_dirty_shm_file_path");
        if (path != NULL) {
            if (attach_to_dirty_shm(path) == 0) {
                dirty_err_count = 0;
                *flag = tm_dirty_handle[0]->flag;
            } else {
                return -1;
            }
        } else {
            if (dirty_err_count++ > 10)
                return -1;
            ERROR_RETURN(("shm not inited, need to call function"
                            " tm_dirty_daemon() or attach to dirty shm"
                            "or set tm_dirty_use_dirty_logical to [1] in bench.conf"), -1);
        }
    }
    return 0;
}
/**
 * @brief 脏词检测总入口 return immediately when detecting dirty word
 * @param level: 不使用，兼容接口
 * @param msg: 待检测的消息
 * @return		0:	clean;
 * 				1:	has dirty words when just detecting A-Z and a-z; second round
 * 				2:	has dirty words when eliminate A-Z and a-z; third round
 *				3:	has dirty words in original message; first round(混合检测更符合实际情况,所以先检测)
 * 				-1: 出错,dirty handle为空，或者daemon函数未调用,或检测串超长(error_log中有详情).
 */
int tm_dirty_check_jump_tag(int level, char *msg, int jump_tag)
{
    int flag = 0;

    if (common_routine(&flag) == -1)
        return -1;

	if (tm_dirty_handle[flag] == NULL)
		return -1;

	return acsmSearch(tm_dirty_handle[flag], (unsigned char *)msg, strlen(msg), jump_tag);
}

int tm_dirty_check(int level, char *msg)
{
    return tm_dirty_check_jump_tag(level, msg, 0);
}

/**
 * @brief 将msg中的脏词全部替换成字符‘*’
 * @param msg: 待检测的消息
 * @return		0:	clean;没有检测到脏词
 * 				1:	有脏词并得到替换 msg被修改为替换后的msg
 * 				-1: 出错,dirty handle为空，或者daemon函数未调用,或检测串超长.
 */
int tm_dirty_replace_jump_tag(char *msg, int jump_tag)
{
	int flag = 0;
    if (common_routine(&flag) == -1)
        return -1;

    if (tm_dirty_handle[flag] == NULL)
		return -1;

	return acsm_pattern_replace(tm_dirty_handle[flag], (unsigned char *)msg, strlen(msg), jump_tag);
}

int tm_dirty_replace(char *msg)
{
    return tm_dirty_replace_jump_tag(msg, 0);
}

/**
 * @brief 脏词检测总入口 return immediately when detecting dirty word并返回检测到的第一个脏词
 * @param level: 不使用，兼容接口
 * @param msg: 待检测的消息
 * @param retbuf: 检测到的脏词存储缓冲区
 * @return		0:	clean;
 * 				1:	has dirty words when just detecting A-Z and a-z; second round
 * 				2:	has dirty words when eliminate A-Z and a-z; third round
 *				3:	has dirty words in original message; first round(混合检测更符合实际情况,所以先检测)
 * 				-1: 出错,dirty handle为空，或者daemon函数未调用,或检测串超长(error_log中有详情).
 */
int tm_dirty_check_where_jump_tag(int level, char *msg, char *retbuf, int jump_tag)
{
	int flag = 0;
	if (common_routine(&flag) == -1)
        return -1;

    if (tm_dirty_handle[flag] == NULL)
        return -1;

	return acsm_get_first_dirty_word(tm_dirty_handle[flag], 
            (unsigned char *)msg, strlen(msg), (unsigned char *)retbuf, jump_tag);
}

int tm_dirty_check_where(int level, char *msg, char *retbuf)
{
    return tm_dirty_check_where_jump_tag(level, msg, retbuf, 0);
}

/**
 * @brief print出内存中load到的脏词配置(用UTF-8终端查看)
 * @param char_set: 不使用，兼容接口
 * @return -1:错误 0:正确
 */
int tm_dirty_list_word(int char_set)
{
	int flag = 0;
	if (common_routine(&flag) == -1)
        return -1;

	if (tm_dirty_handle[flag] != NULL) {
		int count;
		ACSM_PATTERN *list;
		for (count = 0; count < tm_dirty_handle[flag]->total_pattern; count++) {
			list = &(tm_dirty_handle[flag]->acsmPatterns[count]);
			DEBUG_LOG("[%5d]\t<=%s=>", count, list->patrn);
		}
		if (count == 0)
			DEBUG_LOG("no dirty words found but dirty_handle is exist");
	} else {
	    DEBUG_LOG("no dirty words found and dirty_handle is [NOT] exist!!");
	}
	return 0;
}

/**
 * @brief 返回脏词数
 * @return 脏词数 -1表示错误
 */
int tm_dirty_word_count(void)
{
	int flag = 0;
	if (common_routine(&flag) == -1)
        return -1;

	if (tm_dirty_handle[flag] != NULL) {
		return tm_dirty_handle[flag]->total_pattern;
	} else {
		return 0;
	}
}

/**
 * @brief 释放脏词
 * @return 0:free ok; -1: error
 */
int tm_dirty_free()
{
	int i;
    if (loop_thread != 0) {
        if (pthread_cancel(loop_thread) != 0) {
            ERROR_RETURN(("tm_dirty_free[cancel thread]:%s", strerror(errno)), -1);

        }
    }

	for (i = 0; i <= 1; i++) {
		if (tm_dirty_handle[i] != NULL) {
			acsmFree(tm_dirty_handle[i]);
			tm_dirty_handle[i] = NULL;
		}
	}
	return 0;
}

/****************added bye peterhuang**************/
ACSM_STRUCT tm_dirty_handle_info[2];
int attach_to_dirty_shm(char* tm_dirty_file)
{
    init_xlatcase();
    int fd; 
    fd = open(tm_dirty_file, O_RDONLY , 0644);
    if (fd == -1) {
        if(dirty_err_count++ > 10)
            return -1;
        ERROR_RETURN(("tm_dirty: open the shared memory file failed!"), -1);    
    }

    char *p;
    int len= (sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN + STATE_TABLE_LEN);
    p = mmap(NULL, len*2, PROT_READ, MAP_SHARED, fd, 0);// (sizeof (ACSM_STRUCT));
    if (p == MAP_FAILED) {
        if (dirty_err_count++ > 10)
            return -1;
        ERROR_RETURN(("ATTACH TO DIRTY SHM FAILED!"),-1);
    }

    tm_dirty_inner_handle[0] = (ACSM_STRUCT*)p;
    memcpy(&tm_dirty_handle_info[0], tm_dirty_inner_handle[0], sizeof(ACSM_STRUCT));
    tm_dirty_handle_info[0].acsmPatterns = (ACSM_PATTERN*)(p + sizeof(ACSM_STRUCT));
    tm_dirty_handle_info[0].acsmStateTable = (ACSM_STATETABLE*)(p + sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN);
    tm_dirty_inner_handle[1] = (ACSM_STRUCT*)(p + len);
    memcpy(&tm_dirty_handle_info[1], tm_dirty_inner_handle[1], sizeof(ACSM_STRUCT));
    tm_dirty_handle_info[1].acsmPatterns = (ACSM_PATTERN*)(p + len + sizeof(ACSM_STRUCT));
    tm_dirty_handle_info[1].acsmStateTable = (ACSM_STATETABLE*)(p + len + sizeof(ACSM_STRUCT) + PATTERNS_TABLE_LEN + ADS_TABLE_LEN);
   

    tm_dirty_handle[0] = &tm_dirty_handle_info[0];
    tm_dirty_handle[1] = &tm_dirty_handle_info[1];
    
    close(fd);
    daemon_called = 1;
    DEBUG_LOG("ATTACH TO DIRTY SHM OK!");
    return 0;
}


//==========================旧接口
/**
 * @brief 去掉 s 头尾的空白符(空格/tab/换行)
 */
int trim_blank(char *s)
{
	int i, l, r, len;

	for (len = 0; s[len]; len++);
	for (l = 0; (s[l] == ' ' || s[l] == '\t' || s[l] == '\n'); l++);

	if (l == len) {
		s[0] = '\0';
		return 0;
	}

	for (r = len - 1; (s[r] == ' ' || s[r] == '\t' || s[r] == '\n'); r--);
	for (i = l; i <= r; i++) s[i-l] = s[i];

	s[r - l + 1] = '\0';
	return 0;
}

