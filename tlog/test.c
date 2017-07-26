#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tlog.h"


uint32_t get_uid_by_player(void)
{
	return 82333;
}

int main(int argc, char **argv)
{
	int loop_times = 0;
	if (argc != 2) {
		printf("Usage: %s <loop_times>\n", argv[0]);
		return -1;
	}
	loop_times = atoi(argv[1]);

	INIT_DEFAULT_LOGGER_SYSTEM("./log", // logdir: string
							   "1", // prefix: string
							   "seer", // svcname: string
							   2, // gameid: int
							   "online" // svrtype: string
							   );
	SET_LOG_LEVEL(tlog_lvl_trace);

#if 0
	while (loop_times--) {
		uint32_t flag = logger_flag_file | logger_flag_udp;
		write_to_logger(tlog_lvl_debug, "LAST", 82333, flag, "%d, The world is over!\n", i++);
		write_to_logger(tlog_lvl_trace, "LAST", 10086, flag, "%d, The world is over!\n", i++);
		write_to_logger(tlog_lvl_utrace, "LAST", 50001, flag, "%d, The world is over!\n", i++);
		sleep(1);
	}

	int i = 0;
	/* 测试死循环导致的流量暴涨的安全性 */
	while (1) {
		uint32_t flag = logger_flag_file;
		write_to_logger(tlog_lvl_debug, "LAST", 82333, flag, "%d, heavy log: The world is over!\n", i++);
	}
#endif
	BOOT_TLOG("I'm BOOT_TLOG");

	int c;


	c = 100;
	while (c--) 
		LONG_TLOG("add_item", get_uid_by_player(), "LONG_TLOG, no: %d", c);

	c = 100;
	while (c--) 
		UTRACE_TLOG(get_uid_by_player(), "UTRACE_TLOG, no: %d", c);

	c = 100;
	while (c--) 
		FATAL_TLOG("FATAL_TLOG, no: %d", c);

	c = 100;
	while (c--) 
		ERROR_TLOG("ERROR_TLOG, no: %d", c);

	c = 100;
	while (c--) 
		WARN_TLOG("WARN_TLOG, no: %d", c);

	c = 100;
	while (c--) 
		INFO_TLOG("INFO_TLOG, no: %d", c);

	c = 100;
	while (c--) 
		DEBUG_TLOG("DEBUG_TLOG, no: %d", c);

	c = 100;
	while (c--) 
		TRACE_TLOG("TRACE_TLOG, no: %d", c);

#define TEST_SPEC(lvl) \
	do { \
		int cc = 100; \
		while (cc--) { \
			lvl ## _TLOG_EVERY_N(10, #lvl"_TLOG_EVERY_10, no: %d", cc); \
			lvl ## _TLOG_FIRST_N(5, #lvl"_TLOG_FIRST_10, no: %d", cc); \
		} \
	} while (0)

	TEST_SPEC(FATAL);
	TEST_SPEC(ERROR);
	TEST_SPEC(WARN);
	TEST_SPEC(INFO);
	TEST_SPEC(DEBUG);
	TEST_SPEC(TRACE);

#undef TEST_SPEC

#define TEST_SPEC(cond, lvl) \
	do { \
		lvl ## _TLOG_IF(cond, #lvl"_TLOG_IF("#cond")"); \
	} while (0)

	TEST_SPEC(0>1, FATAL);
	TEST_SPEC(1>0, FATAL);

	TEST_SPEC(0>1, ERROR);
	TEST_SPEC(1>0, ERROR);

	TEST_SPEC(0>1, WARN);
	TEST_SPEC(1>0, WARN);

	TEST_SPEC(0>1, INFO);
	TEST_SPEC(1>0, INFO);

	TEST_SPEC(0>1, DEBUG);
	TEST_SPEC(1>0, DEBUG);

	TEST_SPEC(0>1, TRACE);
	TEST_SPEC(1>0, TRACE);

#undef TEST_SPEC

	EXIT_FATAL_TLOG(-1, "EXIT_FATAL_TLOG");

	return 0;
}
