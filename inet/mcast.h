/**
 *============================================================
 *  @file      mcast.h
 *  @brief     多播（组播）相关函数。为了更好地理解和使用这些函数，建议认真阅读一下APUE2第21章 - Multicasting。
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_MCAST_H_
#define LIBTAOMEE_MCAST_H_

#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief create_mcast_socket函数的flag参数取值范围
 */
typedef enum mcast_flag {
	/*! 创建的组播socket只用于读取组播包 */
	mcast_rdonly = 1,
	/*! 创建的组播socket只用于发送组播包 */
	mcast_wronly = 2,
	/*! 创建的组播socket既能读取组播包，也能发送组播包 */
	mcast_rdwr   = mcast_rdonly | mcast_wronly
} mcast_flag_t;

/**
 * @brief 创建一个组播socket。目前只支持IPv4。
 * @param mcast_addr 组播IP地址。只能使用239.X.X.X网段的组播地址。
 * @param port 组播端口。
 * @param nif 网卡接口（eth0/eth1/lo...）。用于指定通过哪个网卡接口收发组播包。
 * @param flag 指定组播socket的用途。mcast_rdonly表示创建的组播socket只用于读取组播包；
 *             mcast_wronly表示创建的组播socket只用于发送组播包；
 *             mcast_rdwr表示创建的组播socket既能读取组播包，也能发送组播包。
 * @param ss 返回初始化好的sockaddr_in/sockaddr_in6结构体，用于sendto发送组播包。
 * @param addrlen 返回ss的有效大小（字节），供sendto使用。
 * @return 成功返回创建好的组播socket fd，失败返回-1。
 */
int create_mcast_socket(const char* mcast_addr, const char* port, const char* nif,
						mcast_flag_t flag, struct sockaddr_storage* ss, socklen_t* addrlen);

/**
  * @brief 加入一个多播组。
  *
  * @param sockfd  加入到多播组的sockfd。成功加入多播组后，应用程序可以通过这个sockfd来获取来自grp的多播包。
  * @param grp     想要加入的多播地址。
  * @param grplen  grp的大小（byte）。
  * @param ifname  加入到多播组的网络接口，比如eth1。成功加入多播组后，内核将使用这个接口来接收来自grp的多播包。
  *
  * @see mcast_leave
  *
  * @return int, 0 成功, -1 失败。
  */
int mcast_join(int sockfd, const struct sockaddr* grp, socklen_t grplen, const char* ifname);

/**
  * @brief 离开一个多播组。
  *
  * @param sockfd  离开多播组的sockfd。成功离开多播组后，应用程序将不再可以通过这个sockfd来获取来自grp的多播包。
  * @param grp     想要离开的多播地址。
  * @param grplen  grp的大小（byte）。
  * @param ifname  离开多播组的网络接口，比如eth1。成功离开多播组后，内核将不再使用这个接口来接收来自grp的多播包。
  *
  * @see mcast_join
  *
  * @return int, 0 成功, -1 失败。
  */
int mcast_leave(int sockfd, const struct sockaddr* grp, socklen_t grplen, const char* ifname);

/**
  * @brief 设置组播包的网络出口。
  *
  * @param sockfd  用来发多播包的sockfd。
  * @param family  sockfd的address family。
  * @param ifname  多播包的网络出口，比如eth1。
  *
  * @return int, 0 成功, -1 失败。
  */
int mcast_set_if(int sockfd, int family, const char* ifname);
// TODO - Returns: non-negative interface index if OK, -1 on error 
int mcast_get_if(int sockfd, int family);

/**
  * @brief 设置开启/禁止组播包的local loopback。默认是开启的。
  *
  * @param sockfd  用来发多播包的sockfd。
  * @param family  sockfd的address family。
  * @param onoff   0禁止，1开启。
  *
  * @see mcast_get_loop
  *
  * @return int, 0 成功, -1 失败。
  */
int mcast_set_loop(int sockfd, int family, int onoff);

/**
  * @brief 获取当前local loopback的设定情况。
  *
  * @param sockfd  用来发多播包的sockfd。
  * @param family  sockfd的address family。
  *
  * @see mcast_set_loop
  *
  * @return int, 成功则返回当前的loopback设定，失败则返回-1。
  */
int mcast_get_loop(int sockfd, int family);

/**
  * @brief 设置发送出去的多播包的ttl次数（ipv4）或者的hop次数（ipv6）。默认次数是1，即限制多播包只能在内网内传播。
  *
  * @param sockfd  用来发多播包的sockfd。
  * @param family  sockfd的address family。
  * @param ttl     ttl/hop次数。
  *
  * @see mcast_get_ttl
  *
  * @return int, 0 成功，-1 失败。
  */
int mcast_set_ttl(int sockfd, int family, int ttl);

/**
  * @brief 获取发送出去的多播包的ttl次数（ipv4）或者的hop次数（ipv6）。
  *
  * @param sockfd  用来发多播包的sockfd。
  * @param family  sockfd的address family。
  *
  * @see mcast_set_ttl
  *
  * @return int, 成功则返回ttl/hop次数，失败则返回-1。
  */
int mcast_get_ttl(int sockfd, int family);
//Returns: current TTL or hop limit if OK, -1 on error

//---------------------------------------------
// TODO
//Block receipt of traffic on this socket from a source given an existing any-source group membership on a specified local interface. 
//
// TODO - add an argument named ifname
// 0 成功, -1 失败。
int mcast_block_source(int sockfd, const struct sockaddr* src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen);
 
int mcast_unblock_source(int sockfd, const struct sockaddr* src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen);
 
int mcast_join_source_group(int sockfd, const struct sockaddr* src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen, const char* ifname);
 
int mcast_leave_source_group(int sockfd, const struct sockaddr *src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen);

#ifdef __cplusplus
}
#endif

#endif // LIBTAOMEE_MCAST_H_
