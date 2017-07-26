#ifndef __TLOG_DECL_H__
#define __TLOG_DECL_H__

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>


#if 0
#define DP(_fmt, _args...) \
	do { \
		printf(_fmt"\n", ##_args); \
	} while (0)
#endif
#define DP(_fmt, _args...) do {} while (0)


#define tlog_likely(x) __builtin_expect(!!(x), 1)
#define tlog_unlikely(x) __builtin_expect(!!(x), 0)


/*! 最大可配置测试uid列表数量 (超过即不可设定) */
#define MAX_TRACE_UID_NUM		(8)

/*! 单个log文件的最大上限 (配置里不能超过这个大小) */
/*! 如果设置 2048 MB, 有可能超过2G是的文件超过系统允许最大大小
 * (未设置 LARGEFILE 的情况下) */
#define MAX_ONE_LOG_SIZE		(2000)

/*! 单个log文件的最小下限 (配置里不能小于这个大小) */
#define MIN_ONE_LOG_SIZE		(10) // 10 MB

/*! 用于限定一类log的每日文件上限 (即 max_files 的最大值); */
#define DAILY_MAX_FILES			(100000)

/*! 不能超过4个十进制位 */
#define MAX_SLICE_NO			(9999)

/*! 每行log的最大长度 (超过则在第8191个字节处被截断, 并把第8192个字节改成\n) */
#define TLOG_BUF_SIZE			(8*1024)

#define MAX_PREFIX_LEN			(16)
#define MAX_PROCNAME_LEN		(64)
#define MAX_HOSTNAME_LEN		(64)
#define MAX_USERNAME_LEN		(32)
#define MAX_TIMESTR_LEN			(64)
#define MAX_SVC_NAME_LEN		(32)
#define MAX_SVRTYPE_LEN			(32)
#define MAX_BASENAME_LEN		(MAX_PROCNAME_LEN + 16) // procname + severity
#define MAX_INFOSTR_LEN			(MAX_SVC_NAME_LEN + MAX_SVRTYPE_LEN + MAX_HOSTNAME_LEN)

/*! 当磁盘满时, 间隔多久执行下一次检查, 单位: 秒 */
#define CHECK_DISKFULL_DUR		(10)

/*! 当遇到任何不能取到的值时(eg: 业务名), 就用这个宏的字符串替代 */
#define NIL						"nil"


/*! 不要切换到新的logfile */
#define DONT_SHIFT				(0)
/*! 要切换到新的logfile */
#define DO_SHIFT				(1)

/*! 注册一个trace_uid时最短的life_time, 单位: 秒 */
#define MIN_TRACE_TIME			(3)
/*! 注册一个trace_uid时最长的life_time, 单位: 秒 */
#define MAX_TRACE_TIME			(86400)


/*! 最大项目编号 */
#define MAX_GAMEID				(1000)


/*! 当初始化 udp_sink 失败时, 间隔多久执行下一次检查, 单位: 秒 */
#define MIN_WAIT_UDP_SINK_TIME	(2)

#define TLOG_ETH0				"eth0"
#define TLOG_ETH1				"eth1"
#define TLOG_ETH2				"eth2"

#define UDP_SINK_BIND_IP		"239.0.0.100" // for mcast
#define BASE_SINK_BIND_PORT		(31000) // 业务接受 ctrl 指令的端口;
#define UDP_SINK_SEND_PORT		(27182) // e=2.7182...

/*! 最大每个进程 1M Bps, 即: 8M bps (以防止死循环造成的网络拥塞)
 * 并且不可能有多个进程同时trace一个uid的情况, 因此这也就是整个服务范围内的限定 */
#define MAX_RATE_LIMIT			(1024*1024)
/*! 默认速率 (初始化时使用): 1M bps */
#define DEF_RATE_LIMIT			(128*1024)
/*! 最小速率 (修改时不能小于这个值): 8K bps */
#define MIN_RATE_LIMIT			(1024)

#define LOGTYPE_HEAD			"LOGHEAD"



enum logger_status_t {
	logger_status_notinit		= 0,
	logger_status_writing		= 1,
	logger_status_stop			= 2,
};

enum logger_flag_t {
	logger_flag_file			= 0x1,
	logger_flag_udp				= 0x2,

	logger_flag_forceflush		= 0x80000000,
};


/**
 * @brief 日志等级
 */
typedef enum tlog_lvl {
	tlog_lvl_min				= 0,

	/*! 长期保存的log (任何情况下都会打印) */
	tlog_lvl_long				= 0,
	/*! 在线跟踪号码的详细日志 (任何情况下都会打印) */
	tlog_lvl_utrace				= 1,
	/*! 致命错误, 写完fatal就直接退出进程 */
	tlog_lvl_fatal				= 2,
	/*! 严重错误, 导致程序必须对此进一步处理 (例如输入非法, 导致必须立刻返回) */
	tlog_lvl_error				= 3,
	/*! 错误, 程序不对此做处理, 程序也能正确的执行下去 (通常记录一些不合理的设计) */
	tlog_lvl_warn				= 4,
	/*! 非错误, 可记录一些业务信息 (eg: 业务流程数据, 系统统计信息) */
	tlog_lvl_info				= 5,
	/*! 通常用来记录调试信息 */
	tlog_lvl_debug				= 6,
	/*! 开发时用的log (发布时通常对这个级别的设定报警) */
	tlog_lvl_trace				= 7,

	tlog_lvl_max,
} tlog_lvl_t;

enum def_ctrl_cfg_t {
	def_log_level				= tlog_lvl_debug,
	def_daily_max_files			= 0, // 没有每日文件数量上限
	def_max_one_size			= 100, // 每个文件的上限: 100 MB
	def_stop_if_diskfull		= 1, // 当磁盘满时, 停止写文件 (但从udp_sink可发日志)
	def_time_slice_secs			= 900, // 不要每隔多少时间就创建一个新文件, 一直写;
};


/* 业务传递给logger的业务基础信息, 用于log文件名或者log内容 */
typedef struct _logger_svc_info {
	/*! 0: 未初始化, 1: 已初始化 */
	int					init;
	/*! 业务名称 */
	char				svcname[MAX_SVC_NAME_LEN];
	/*! 业务ID */
	int					gameid;
	/*! 服务器类型 (online/db/...) */
	char				svrtype[MAX_SVRTYPE_LEN];

	char				procname[MAX_PROCNAME_LEN]; // 进程名
	char				hostname[MAX_HOSTNAME_LEN]; // 实际是ip
} logger_svc_info_t;

/* 控制logger行为的一些参数 */
typedef struct _logger_ctrl_cfg {
	/*! 0: 未初始化, 1: 已初始化 */
	int					init;
	/*! 通过初始化/动态设定的log级别 */
	tlog_lvl_t			log_level;
	/*! 最大的文件数量 (TODO: 先记录, 还未实现) */
	int32_t				daily_max_files;
	/*! 单个log文件的最大字节 */
	size_t				max_one_size;
	/*! 如果发现磁盘满了, 就把 status 改成 stop, 来停止写log(包括 udp) */
	int					stop_if_diskfull;
	/*! 多少秒log文件切换一次 */
	int32_t				time_slice_secs;
} logger_ctrl_cfg_t;

typedef struct _logfile_info {
	/* logfile的 level */
	tlog_lvl_t			my_lvl;
	/*! logfile的fd */
	int					fd;

	/*! logfile的每日序号 (目前用于支持每天最大文件数检测) */
	int32_t				daily_seq;
	/*! logfile的时间片序号 (目前用于支持按时间片切分log) */
	int32_t				time_slice_seq;
	/*! logfile的时间片切片序号 (目前用于支持按时间片切分log) */
	int32_t				time_slice_no;
	/*! 当前文件所在的日期 (tm->tm_yday) */
	int32_t				tm_yday;

	/*! log文件名的基础部分: <program_name>-<severity> */
	char    			basename[MAX_BASENAME_LEN];
	/*! basename 的实际长度 */
	int     			baselen;

	/*! log文件名的不变部分: <svcname>-<svrtype>-<hostname> */
	char				infostr[MAX_INFOSTR_LEN];
	/*! infostring 的实际长度 */
	int     			infostrlen;

	/*! 文件总长度 (字节数) */
	int64_t				file_len;
} logfile_t;

/**
 * @brief 日志管理结构, 包含: formatter, sink 等信息;
 */
typedef struct _logger {
	/*! 0: not-init, 1: writing, 2: stop-writing */
	uint32_t			status;
	/*! 上一次检查是否可以放开write的时间 (用于当磁盘满时, 恢复写日志) */
	time_t				last_check_diskfull_time;

	/*! 日志保存目录 */
	char				logdir[FILENAME_MAX];
	/*! 日志文件的前缀 */
	char				prefix[MAX_PREFIX_LEN];

	/*! 是否开启网络控制接口, 0: 不开启, 1: 开启 (初始化looger时会创建监听线程) */
	uint8_t				use_net_ctrl;

	/*! udp_sink 用来接收远端请求的线程 */
	pthread_t			thread;

	/*! 缓存当前时间, 每次 write_to_logger 的开始赋值, 与 tm_now 对应 */
	time_t				now;
	/*! 缓存当前时间, 每次 write_to_logger 的开始赋值, 与 now 对应 */
	struct tm			tm_now;

	/*! 日志文件的格式化参数 (按log等级存放) */
	logfile_t			logfile[tlog_lvl_max];

} logger_t;





typedef struct _trace_uid {
	/*! 被追踪的 uid */
	uint32_t			uid;
	/*! 被追踪 uid 的trace持续到的时间 (超过了这个时间, 就自然取消跟踪) */
	int32_t				expire_time;
} trace_uid_t;

typedef struct _rate_limit {
	/*! 流量控制(漏捅): 每秒计算: 监控同一秒内的流量 */
	time_t				last_rate_time;
	/*! 漏捅中的令牌数; */
	int32_t				token;
	/*! 速率上限 (固定成 1MBps 即: 8Mbps), 可在1秒内支持平均 128B 的log 8192条 */
	int32_t				limit;
	/*! 0: 没有修改, 合理值: 表示之前有设定过, 需要把该值在适当的时候赋值给limit */
	int32_t				chg_limit;
} rate_limit_t;

/*! 用于发送log的udp网络接口 */
typedef struct _logger_udp_sink {
	/*! 发出udplog的fd */
	int					sendfd;
	/*! 指向udplog的地址 */
	struct sockaddr_in	toaddr;
	/*! 被远端设置的发送ip */
	char				ip[INET_ADDRSTRLEN];
	/*! 被远端设置的发送port */
	uint16_t			port;
	/*! 主线程在发送时检查这个标志, 如果是非0, 则需要重新构造 toaddr
	 * 这是由于在更新 ip/port时, 不适合同时更新toaddr, 因为主线程可能正在使用 */
	int					addr_changed;

	/*! 流量控制参数 (防止trace_log写得不好, 导致的死循环造成的流量猛增) */
	rate_limit_t		rate_limit;

	trace_uid_t			trace_uid_list[MAX_TRACE_UID_NUM];
	char 				recvbuf[8192];
	char 				sendbuf[8192];
} logger_udp_sink_t;


extern logger_ctrl_cfg_t *ctrl_cfg;
extern logger_svc_info_t *svc_info;
extern logger_udp_sink_t *udp_sink;
extern logger_t *logger;


#endif //__TLOG_DECL_H__
