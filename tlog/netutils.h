#ifndef __NETUTILS_H__
#define __NETUTILS_H__


#include "tlog_decl.h"


enum logger_cmd {
	lc_set_trace_uid		= 1001,
	lc_unset_trace_uid		= 1002,
	lc_set_trace_addr		= 1003,
	lc_set_rate_limit		= 1004,
};


#pragma pack(1)

typedef struct _logger_ctrl_pkg {
	uint32_t			len;
	uint32_t			ver;
	uint32_t			cmd;
	uint32_t			uid;
	uint32_t			seq;
	uint8_t				body[];
} logger_ctrl_pkg_t;

typedef struct _pkg_trace_uid {
	/*! 被追踪的 uid */
	uint32_t			uid;
	/*! 被追踪 uid 的trace时间 (过了时间, 就自然取消跟踪) */
	int32_t				life_time;
} pkg_trace_uid_t;

typedef struct _logger_set_trace_uid {
	/*! 被 trace uid 的数量 */
	uint32_t			count;
	pkg_trace_uid_t		pkg_trace_uid[];
} logger_set_trace_uid_t;

typedef struct _logger_set_trace_addr {
	char				ip[INET_ADDRSTRLEN];
	uint16_t			port;
} logger_set_trace_addr;

typedef struct _logger_set_rate_limit {
	int32_t				limit;
} logger_set_rate_limit;


#pragma pack()




int tlog_set_socket_nonblock(int sock);
void tlog_set_socket_reuseaddr(int sock);
int add_to_trace_uid_list(time_t now, pkg_trace_uid_t *pkg_trace_uid);
int do_set_trace_uid(int fd, time_t now, logger_ctrl_pkg_t *pkg,
		struct sockaddr_in *from, socklen_t fromlen);
void proc_logger_pkg(int fd, time_t now, char *recvbuf, ssize_t len,
		struct sockaddr_in *from, socklen_t fromlen);

void tlog_on_idle(void);

int tlog_mcast_join(int mfd, const struct sockaddr *grpaddr,
		socklen_t grpaddrlen, const char *ethx);
int tlog_create_mcast_socket(const char *mcast_ip, int gameid, const char *ethx);

void *udp_sink_server(void *args);

int update_udp_sink_sockaddr(void);
int init_logger_udp_sink_sendfd(void);

void sendlog_to_udp_sink(const char *buffer, int len);
int tlog_get_local_eth_ip(const char *eth, struct in_addr *addr);
int tlog_get_local_eth_ipstr(const char *eth, char *ipbuf);


#endif //__NETUTILS_H__
