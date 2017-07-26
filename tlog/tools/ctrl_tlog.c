#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "svc.h"
#include "ctrl_tlog.h"


static char proc_param[] = "[-v] [-h] -s svcname -c ctrl -u uid -l lifetime -i sendip -t target -r rate_limit(B/s)";
static char mcast_ip[INET_ADDRSTRLEN] = UDP_SINK_BIND_IP;
static uint16_t mcast_port = 0;
static char cmd_str_buf[128];
static char sendbuf[8192];

typedef void (*ctrl_func_t) (void);
typedef struct _ctrl_opt {
	char				*name;
	ctrl_func_t			func;
} ctrl_opt_t;

static ctrl_opt_t ctrl_opts[] = {
	{ "set_trace_uid", do_set_trace_uid},
	{ "unset_trace_uid", do_unset_trace_uid},
	{ "set_trace_addr", do_set_trace_addr},
	{ "set_rate_limit", do_set_rate_limit},
};
const int ctrl_opts_count = sizeof(ctrl_opts) / sizeof(ctrl_opts[0]);

struct mcast_info_t {
	int mcast_fd;
	struct sockaddr_in mcast_addr;
} mcast_info;

typedef struct _args {
	int					count;
	uint32_t			uid;
	int32_t				life_time;

	char				*ctrl; // 操作名称;
	char				*target; // "mcast": ctrl对全项目执行; "ip": ctrl对ip的机器执行;

	int					gameid; // 业务ID;
	char				*svcname; // 业务名;

	char				*ip; // 给业务设置的发送log的目的ip;
	uint16_t			port; // 给业务设置的发送log的目的ip;

	int32_t				rate_limit;
} args_t;

args_t args_struct, *args = &args_struct;

static struct option long_options[] = {
	/* MUST */
	{"svcname", 1, 0, 's'},
	{"ctrl", 1, 0, 'c'},
	{"target", 1, 0, 't'},

	/* option */
	{"uid", 2, 0, 'u'},
	{"lifetime", 2, 0, 'l'},
	{"ip", 2, 0, 'i'},
	{"port", 2, 0, 'p'},
	{"rate_limit", 2, 0, 'r'},

	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'v'},
};

inline char *cmd_string(uint32_t cmd)
{
	memset(cmd_str_buf, 0, sizeof(cmd_str_buf));
	sprintf(cmd_str_buf, "%s_cmd", long_options[cmd - 1001].name);
	return cmd_str_buf;
}

int make_sockaddr(char *mip, uint16_t port, struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	if (inet_pton(AF_INET, mip, &(addr->sin_addr)) != 1)
		return -1;
	if (port > 65535)
		return -1;
	addr->sin_port = htons(port);
	return 0;
}

int set_tlog_mcast_if(int fd, const char *ethx)
{
	struct ifreq ifreq;

	strncpy(ifreq.ifr_name, ethx, IFNAMSIZ);
	if (ioctl(fd, SIOCGIFADDR, &ifreq) < 0) {
		return -1;
	}

	struct in_addr *inaddr = &((struct sockaddr_in*)&ifreq.ifr_addr)->sin_addr;
	return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, inaddr, sizeof(*inaddr));
}

int get_local_eth_ip(const char *eth, struct in_addr *addr)
{
	int fd;
	struct sockaddr_in *sin;
	struct ifreq ifr;

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket: ");
		return -1;
	}

	/* Get IP Address */
	strncpy(ifr.ifr_name, eth, IF_NAMESIZE);
	ifr.ifr_name[IFNAMSIZ - 1]='\0';

	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
		perror("ioctl: ");
		close(fd);
		return -1;
	}
	close(fd);

	sin = (struct sockaddr_in *)(&ifr.ifr_addr);
	memcpy(addr, &(sin->sin_addr), sizeof(*addr));

	return 0;
}

int get_local_eth_ipstr(const char *eth, char *ipbuf)
{
	struct sockaddr_in sin;

	if (get_local_eth_ip(eth, &(sin.sin_addr)) == -1) {
		return -1;
	}

	if (inet_ntop(AF_INET, &(sin.sin_addr), ipbuf, INET_ADDRSTRLEN) == NULL) {
		perror("inet_ntop: ");
		return -1;
	}

	return 0;
}

int make_mcast_sock(void)
{
	char local_ip[INET_ADDRSTRLEN];
	int ret = get_local_eth_ipstr("eth0", local_ip);
	const char *ethx = "eth1"; // 外网就设置 eth1, 内网就设置 eth0
	if (ret == -1) {
		return -1;
	} else if (strncmp(local_ip, "10.1.", 5) == 0) {
		ethx = "eth0";
	}

	mcast_info.mcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in *mcast_addr = &(mcast_info.mcast_addr);
	set_tlog_mcast_if(mcast_info.mcast_fd, ethx);
	memset(mcast_addr, 0, sizeof(*mcast_addr));
	if (make_sockaddr(mcast_ip, mcast_port, mcast_addr) < 0) {
		fprintf(stderr, "invalid mcast addr: ip=\"%s\", port=%u\n",
				mcast_ip, mcast_port);
		return -1;
	}
	return mcast_info.mcast_fd;
}

#if 0
void send_ctrl_cmd(char *optarg, char *mip, uint16_t port, uint32_t cmd)
{
	memset(sendbuf, 0, sizeof(sendbuf));

	int i;
	struct sockaddr_in *addr = &(mcast_info.mcast_addr);
	int mfd = mcast_info.mcast_fd;

	logger_ctrl_pkg_t *pkg = (logger_ctrl_pkg_t *)sendbuf;
	memset(pkg, 0, sizeof(*pkg));
	pkg->cmd = htonl(cmd);

	uint32_t len = sizeof(logger_ctrl_pkg_t);
	switch (cmd) {
	case lc_set_trace_uid:
	{
		set_trace_uid_args_t args_struct = { 0 };
		set_trace_uid_args_t *args = &args_struct;
		if (parse_optarg_set_trace_uid(args, optarg) < 0) {
			fprintf(stderr, "Failed to parse args: %s\n", optarg);
			return ;
		}
		if (args->count == 0) break;

		logger_set_trace_uid_t *set_trace_uid = (logger_set_trace_uid_t *)pkg->body;
		set_trace_uid->count = htonl(args->count);
		for (i = 0; i < args->count; i++) {
			pkg_trace_uid_t *trace = &(set_trace_uid->pkg_trace_uid[i]);
			trace->uid = htonl(args->pkg_trace_uid[i].uid);
			trace->life_time = htonl(args->pkg_trace_uid[i].life_time);
		}
		len += sizeof(logger_set_trace_uid_t) + args->count * sizeof(pkg_trace_uid_t);
	}
		break;

	case lc_set_trace_addr:
		len += sizeof(logger_set_trace_addr);
		break;

	default:
		printf("Unknown cmd: %u, bye!", cmd);
		return ;
	}
	pkg->len = htonl(len);

	printf("mcast_ip:\"%s\", port=%u, cmd=%s, target_ip=%s\n",
			mip, port, cmd_string(cmd), optarg);
	int err = sendto(mfd, sendbuf, len, 0,
			(const struct sockaddr *)(addr), sizeof(*addr));
	if (err == -1) {
		perror("send failed: ");
	}
}
#endif

void do_set_trace_uid(void)
{
	if (args->uid < 50000) {
		fprintf(stderr, "Invalid uid: %u\n", args->uid);
		return ;
	}

	if (args->life_time < MIN_TRACE_TIME
		|| args->life_time > MAX_TRACE_TIME) {
		fprintf(stderr, "Invalid life_time: %d\n", args->life_time);
		return ;
	}

	int i;
	uint32_t len = sizeof(logger_ctrl_pkg_t);
	struct sockaddr_in *addr = &(mcast_info.mcast_addr);
	int mfd = mcast_info.mcast_fd;

	logger_ctrl_pkg_t *pkg = (logger_ctrl_pkg_t *)sendbuf;
	memset(pkg, 0, sizeof(*pkg));

	args->count = 1; // TODO: 变成一次加多个的
	logger_set_trace_uid_t *set_trace_uid = (logger_set_trace_uid_t *)pkg->body;
	set_trace_uid->count = htonl(args->count);
	for (i = 0; i < args->count; i++) {
		pkg_trace_uid_t *trace = &(set_trace_uid->pkg_trace_uid[i]);
		trace->uid = htonl(args->uid);
		trace->life_time = htonl(args->life_time);
	}
	len += sizeof(logger_set_trace_uid_t) + args->count * sizeof(pkg_trace_uid_t);

	pkg->cmd = htonl(lc_set_trace_uid);
	pkg->len = htonl(len);

	int err = sendto(mfd, sendbuf, len, 0,
			(const struct sockaddr *)(addr), sizeof(*addr));
	if (err == -1) {
		perror("send failed: ");
	}
}

void do_unset_trace_uid(void)
{
	printf("%s is coming soon ...\n", __FUNCTION__);
}

void do_set_trace_addr(void)
{
	if (args->ip == NULL // 不用再次检查 args->gameid 了
		|| args->port == 0) {
		printf("Must set ip(-i) and port(-p) when %s\n", __FUNCTION__);
		return ;
	}

	/* 尝试自己制作一个发送地址 (TODO: 最好模拟发送一个test包, 以确保有效) */
	struct sockaddr_in test_addr;
	if (inet_pton(AF_INET, args->ip, &(test_addr.sin_addr)) != 1) {
		printf("Must invalid ip(-i): %s\n", args->ip);
		return ;
	}

	logger_ctrl_pkg_t *pkg = (logger_ctrl_pkg_t *)sendbuf;
	memset(pkg, 0, sizeof(*pkg));

	logger_set_trace_addr *set_trace_addr = (logger_set_trace_addr *)pkg->body;
	snprintf(set_trace_addr->ip, sizeof(set_trace_addr->ip), "%s", args->ip);
	set_trace_addr->port = htons(args->port);

	uint32_t len = sizeof(logger_ctrl_pkg_t);
	len += sizeof(logger_set_trace_addr);

	pkg->cmd = htonl(lc_set_trace_addr);
	pkg->len = htonl(len);

	int mfd = mcast_info.mcast_fd;
	struct sockaddr_in *addr = &(mcast_info.mcast_addr);
	int err = sendto(mfd, sendbuf, len, 0,
			(const struct sockaddr *)(addr), sizeof(*addr));
	if (err == -1) {
		perror("send failed: ");
	}
}

void do_set_rate_limit(void)
{

	logger_ctrl_pkg_t *pkg = (logger_ctrl_pkg_t *)sendbuf;
	memset(pkg, 0, sizeof(*pkg));

	logger_set_rate_limit *set_rate_limit = (logger_set_rate_limit *)pkg->body;
	set_rate_limit->rate_limit = htonl(args->rate_limit);

	uint32_t len = sizeof(logger_ctrl_pkg_t) + sizeof(logger_set_rate_limit);

	pkg->cmd = htonl(lc_set_rate_limit);
	pkg->len = htonl(len);

	int mfd = mcast_info.mcast_fd;
	struct sockaddr_in *addr = &(mcast_info.mcast_addr);
	int err = sendto(mfd, sendbuf, len, 0,
			(const struct sockaddr *)(addr), sizeof(*addr));
	if (err == -1) {
		perror("send failed: ");
	}
}

int main(int argc, char* argv[])
{
	int c;
	if (argc == 1) {
		print_desc();
		print_usage(argv, proc_param);
		print_ver();
		exit(EXIT_SUCCESS);
	}

	if (load_svc_gameid_map(NULL, NULL) == -1) {
		fprintf(stderr, "Failed to load_svc_gameid_map\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "s:c:t:u:l:i:p:r:vh",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 's': // svcname
			args->svcname = strdup(optarg);
			break;
		case 'c': // ctrl
			args->ctrl = strdup(optarg);
			break;
		case 't': // target: 执行ctrl指令的业务机器 (mcast 为全部业务机器)
			args->target = strdup(optarg);
			break;

		case 'u': // uid
			args->uid = atoi(optarg);
			break;
		case 'l': // lifetime
			args->life_time = atoi(optarg);
			break;
		case 'i': // ip: tlog发送到的ip (必须与port一起提供)
			args->ip = strdup(optarg);
			break;
		case 'p': // ip: tlog发送到的port (必须与ip一起提供)
			args->port = atoi(optarg);
			break;
		case 'r': // rate_limit: 发送速率 (单位: 字节/秒)
			args->rate_limit = atoi(optarg);
			break;

		case 'h': // help
			print_usage(argv, proc_param);
			break;
		case 'v': // version
			print_ver();
			break;
		case '?': // invalid args
			break;
		default: /* unsupported options */
			printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	args->gameid = get_svc_gameid(args->svcname);
	if (args->gameid <= 0 || args->gameid > MAX_SVC_NUM) {
		printf("Unknown svc: %s\n", args->svcname);
		exit(EXIT_FAILURE);
	}
	mcast_port = BASE_SINK_BIND_PORT + args->gameid;

	memset(&mcast_info, 0, sizeof(mcast_info));
	if (make_mcast_sock() < 0) {
		printf("Failed to create mcast_ip: %s, mcast_port: %hu\n",
				mcast_ip, mcast_port);
		exit(EXIT_FAILURE);
	}

	int i = 0;
	for (; i < ctrl_opts_count; i++) {
		if (!strcmp(args->ctrl, ctrl_opts[i].name)) {
			ctrl_opts[i].func();
			break;
		}
	}

	close(mcast_info.mcast_fd);
	exit(EXIT_SUCCESS);
}
