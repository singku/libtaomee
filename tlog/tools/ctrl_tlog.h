#ifndef __CTRL_TLOG_H__
#define __CTRL_TLOG_H__

#include <arpa/inet.h>
#include <stdint.h>
#include "svc.h"

#define MJR_VER 0
#define MIN_VER 80
#define PROG_DESC "A tool for remote set trace-uid for tlog"

#define DIM(a) (sizeof(a) / sizeof(a[0]))

#define print_ver() \
	do { \
		printf("Version: \033[1;5;31m%d.%d\033[0m\n", MJR_VER, MIN_VER); \
	} while(0) 

#define print_usage(argv, param) \
	do {\
		printf("Usage: %s %s\n", argv[0], param);\
		printf("-h: Help(this info list)\n"); \
		printf("-v: Version\n"); \
		printf("You can also use long_param:\n"); \
		uint32_t i = 0; \
		for (; i < DIM(long_options); i++) {\
			if (long_options[i].val == 'h' || long_options[i].val == 'v') \
				continue; \
			printf("\t-%c = --%s\n", long_options[i].val, long_options[i].name); \
		} \
		printf("\n\tSupported ctrl's names:\n"); \
			int j; \
			for (j = 0 ; j < ctrl_opts_count; j++) { \
				printf("\t\t%s\n", ctrl_opts[j].name); \
			} \
	} while(0)

#define print_desc() \
	do { \
		printf("\033[1;37m" "%s\n\n" "\033[0m", PROG_DESC); \
	} while(0)

#define print_func_name() do { printf("[%s]\n", __FUNCTION__); } while(0)


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
	int32_t				rate_limit;
} logger_set_rate_limit;


#pragma pack()

/*! 最大项目编号 */
#define MAX_GAMEID				(1000)
/*! 注册一个trace_uid时最短的life_time, 单位: 秒 */
#define MIN_TRACE_TIME			(3)
/*! 注册一个trace_uid时最长的life_time, 单位: 秒 */
#define MAX_TRACE_TIME			(86400)


/*! 最大可配置测试uid列表数量 (超过即不可设定) */
#define MAX_TRACE_UID_NUM		(8)


#define UDP_SINK_BIND_IP		"239.0.0.100" // for mcast
#define BASE_SINK_BIND_PORT		(31000) // 业务接受 ctrl 指令的端口;
#define UDP_SINK_SEND_PORT		(27182) // e=2.7182...



void do_set_trace_uid(void);
void do_unset_trace_uid(void);
void do_set_trace_addr(void);
void do_set_rate_limit(void);

#endif /* __CTRL_TLOG_H__ */
