/**
 *============================================================
 *  @file      inet_utils.h
 *  @brief    
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_INET_UTILS_H_
#define LIBTAOMEE_INET_UTILS_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 用于通过网卡接口（eth0/eth1/lo...）获取对应的IP地址。支持IPv4和IPv6。
 * @param nif 网卡接口。eth0/eth1/lo...
 * @param af 网络地址类型。AF_INET或者AF_INET6。
 * @param ipaddr 用于返回nif和af对应的IP地址。ipaddr的空间由函数调用者分配，并且长度必须大于或者等于IP地址的长度（16或者46字节）。
 * @param len ipaddr的长度（字节）。
 * @return 成功返回0，并且ipaddr中保存了nif和af对应的IP地址。失败返回-1。
 */
int get_ip_addr(const char* nif, int af, void* ipaddr, size_t len);

/**
 * @brief 把getnameinfo、getaddrinfo等函数返回的EAI_XXX错误码转换成类似的EXXX(errno)错误码。
 * @param eai EAI_XXX错误码
 * @return 返回类似的EXXX(errno)错误码
 */
int eai_to_errno(int eai);

/**
 * @brief 判断给定的str_ip点分Ip串是否一个合法的ipv4地址
 * @param str_ip ipv4地址串
 * @return 0:不是；1合法的Ipv4地址
 */
int is_legal_ip(const char *str_ip);

/**
 * @brief 判断给定的port是否一个合法的端口
 * @param port int 整形表示的端口
 * @return 0:不是；1合法的端口
 */
int is_legal_port(const int port);

#ifdef __cplusplus
}
#endif

#endif // LIBTAOMEE_INET_UTILS_H_

