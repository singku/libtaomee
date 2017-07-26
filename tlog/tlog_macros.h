#ifndef __TLOG_MACROS_H__
#define __TLOG_MACROS_H__

/*
 * tlog库的接口, 通过一些列的宏的封装的形式提供;
 * 绝大多数情况下, tlog库的用户应该使用本文件里的接口,
 * 而不要调用更底层的接口
 *
 * 说明 / 约定:
 * 1. 以 "RT_" 开头的所有的接口, 第一个参数(rt)一定是被return的返回值,
 * 		并且该接口用 "return rt"; 返回到上一层调用;
 * 		注意: 返回值(rt)只能是int型整数;
 *
 * 2. 以 "RTV_" 开头的所有的接口, 没有 rt 参数, 但会用 "return" 返回到上一层调用;
 *
 * 3. 以 "EXIT_" 开头的所有接口, 第一个参数(ex)一定是被 exit() 的返回值,
 * 		并且该接口用 "exit(ex);" 退出整个进程;
 * 		注意: 返回值(ex)只能是int型整数;
 */


/* ========================================================================== */
/* tlog helpers */
/* ========================================================================== */

#define TLOG_EVERY_N_VARNAME_CONCAT(base, line) base ## line
#define TLOG_EVERY_N_VARNAME(base, line) TLOG_EVERY_N_VARNAME_CONCAT(base, line)
#define TLOG_LINE_VARNAME TLOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define TLOG_LINE_VARNAME_MOD_N TLOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#define TLOG_EVERY_N(lvl, n, flag, fmt, args...) \
	do { \
		static int TLOG_LINE_VARNAME = 0, TLOG_LINE_VARNAME_MOD_N = 0; \
		++TLOG_LINE_VARNAME; \
		if (++TLOG_LINE_VARNAME_MOD_N > n) TLOG_LINE_VARNAME_MOD_N -= n; \
		if (TLOG_LINE_VARNAME_MOD_N == 1) { \
			write_to_logger(lvl, NULL, 0, flag, fmt, ##args); \
		} \
	} while (0)

#define TLOG_FIRST_N(lvl, n, flag, fmt, args...) \
	do { \
		static int TLOG_LINE_VARNAME = 0; \
		if (TLOG_LINE_VARNAME < n) {\
			++TLOG_LINE_VARNAME; \
			write_to_logger(lvl, NULL, 0, flag, fmt, ##args); \
		} \
	} while (0)

#define TLOG_IF(lvl, condition, flag, fmt, args...) \
	!(condition) ? (void) 0 : write_to_logger(lvl, NULL, 0, flag, fmt, ##args)



/* ========================================================================== */
/* BOOT_TLOG */
/* ========================================================================== */

/* eg: BOOT_TLOG("dlopen error, %s", error); */
#define BOOT_TLOG(fmt, args...) \
	do { \
		boot_tlog(0, 0, fmt, ##args); \
	} while (0)


/* eg: RT_BOOT_TLOG(-1, "dlopen error, %s", error); */
#define RT_BOOT_TLOG(rt, fmt, args...) \
	do { \
		boot_tlog(rt, 0, fmt, ##args); \
		return rt; \
	} while (0)

#define SCREEN_TLOG(rt, fmt, args...) \
    do { \
        std_tlog(rt, fmt, ##args);\
    } while (0)

#define RT_SCREEN_TLOG(rt, fmt, args...) \
    do { \
        std_tlog(rt, fmt, ##args);\
        return rt;\
    } while (0)


/* ========================================================================== */
/* LONG_TLOG */
/* ========================================================================== */

/* eg: LONG_TLOG("add_item", 82333, "debug: %s", msg); */
#define LONG_TLOG(logtype, uid, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_long, logtype, uid, logger_flag_file, fmt"\n", ##args); \
	} while (0)



/* ========================================================================== */
/* UTRACE_TLOG */
/* ========================================================================== */

/* eg: UTRACE_TLOG(82333, "Info: %s", msg); */
#define UTRACE_TLOG(uid, fmt, args...) \
	do { \
		if (is_traced_uid(uid)) { \
			uint32_t __flag = logger_flag_file | logger_flag_udp; \
			write_to_logger(tlog_lvl_utrace, NULL, uid, __flag, fmt"\n", ##args); \
		} \
	} while (0)



/* ========================================================================== */
/* FATAL_TLOG */
/* ========================================================================== */

/* eg: FATAL_TLOG("%s", msg); */
#define FATAL_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_fatal, NULL, 0, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)

/* eg: RT_FATAL_TLOG(-1, "%s", msg); */
#define RT_FATAL_TLOG(rt, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_fatal, NULL, 0, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
		return rt; \
	} while (0)

/* eg: RTV_FATAL_TLOG("%s", msg); */
#define RTV_FATAL_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_fatal, NULL, 0, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
		return ; \
	} while (0)

/* eg: EXIT_FATAL_TLOG(-1, "%s", msg); */
#define EXIT_FATAL_TLOG(ex, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_fatal, NULL, 0, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
		exit(ex); \
	} while (0)

/* eg: FATAL_TLOG_EVERY_N(100, "%s", msg); */
#define FATAL_TLOG_EVERY_N(n, fmt, args...) \
	do { \
		TLOG_EVERY_N(tlog_lvl_fatal, n, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)

/* eg: FATAL_TLOG_FIRST_N(10, "%s", msg); */
#define FATAL_TLOG_FIRST_N(n, fmt, args...) \
	do { \
		TLOG_FIRST_N(tlog_lvl_fatal, n, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)

/* eg: FATAL_TLOG_IF(!(uid > 50000), "%s", msg); */
#define FATAL_TLOG_IF(cond, fmt, args...) \
	do { \
		TLOG_IF(tlog_lvl_fatal, cond, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)



/* ========================================================================== */
/* ERROR_TLOG */
/* ========================================================================== */

/* eg: ERROR_TLOG("%s", msg); */
#define ERROR_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_error, NULL, 0, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)

/* eg: RT_ERROR_TLOG(-1, "%s", msg); */
#define RT_ERROR_TLOG(rt, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_error, NULL, 0, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
		return rt; \
	} while (0)

/* eg: RTV_ERROR_TLOG("%s", msg); */
#define RTV_ERROR_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_error, NULL, 0, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
		return ; \
	} while (0)

/* eg: ERROR_TLOG_EVERY_N(100, "%s", msg); */
#define ERROR_TLOG_EVERY_N(n, fmt, args...) \
	do { \
		TLOG_EVERY_N(tlog_lvl_error, n, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)

/* eg: ERROR_TLOG_FIRST_N(10, "%s", msg); */
#define ERROR_TLOG_FIRST_N(n, fmt, args...) \
	do { \
		TLOG_FIRST_N(tlog_lvl_error, n, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)

/* eg: ERROR_TLOG_IF(!(uid > 50000), "%s", msg); */
#define ERROR_TLOG_IF(cond, fmt, args...) \
	do { \
		TLOG_IF(tlog_lvl_error, cond, logger_flag_file, "[%s][%d]%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	} while (0)



/* ========================================================================== */
/* WARN_TLOG */
/* ========================================================================== */

/* eg: WARN_TLOG("%s", msg); */
#define WARN_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_warn, NULL, 0, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: RT_WARN_TLOG(-1, "%s", msg); */
#define RT_WARN_TLOG(rt, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_warn, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return rt; \
	} while (0)

/* eg: RTV_WARN_TLOG("%s", msg); */
#define RTV_WARN_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_warn, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return ; \
	} while (0)

/* eg: WARN_TLOG_EVERY_N(100, "%s", msg); */
#define WARN_TLOG_EVERY_N(n, fmt, args...) \
	do { \
		TLOG_EVERY_N(tlog_lvl_warn, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: WARN_TLOG_FIRST_N(10, "%s", msg); */
#define WARN_TLOG_FIRST_N(n, fmt, args...) \
	do { \
		TLOG_FIRST_N(tlog_lvl_warn, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: WARN_TLOG_IF(!(uid > 50000), "%s", msg); */
#define WARN_TLOG_IF(cond, fmt, args...) \
	do { \
		TLOG_IF(tlog_lvl_warn, cond, logger_flag_file, fmt"\n", ##args); \
	} while (0)



/* ========================================================================== */
/* INFO_TLOG */
/* ========================================================================== */

/* eg: INFO_TLOG("%s", msg); */
#define INFO_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_info, NULL, 0, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: RT_INFO_TLOG(0, "%s", msg); */
#define RT_INFO_TLOG(rt, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_info, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return rt; \
	} while (0)

/* eg: RTV_INFO_TLOG("%s", msg); */
#define RTV_INFO_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_info, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return ; \
	} while (0)

/* eg: INFO_TLOG_EVERY_N(100, "%s", msg); */
#define INFO_TLOG_EVERY_N(n, fmt, args...) \
	do { \
		TLOG_EVERY_N(tlog_lvl_info, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: INFO_TLOG_FIRST_N(10, "%s", msg); */
#define INFO_TLOG_FIRST_N(n, fmt, args...) \
	do { \
		TLOG_FIRST_N(tlog_lvl_info, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: INFO_TLOG_IF(!(uid > 50000), "%s", msg); */
#define INFO_TLOG_IF(cond, fmt, args...) \
	do { \
		TLOG_IF(tlog_lvl_info, cond, logger_flag_file, fmt"\n", ##args); \
	} while (0)



/* ========================================================================== */
/* DEBUG_TLOG */
/* ========================================================================== */

/* eg: DEBUG_TLOG("%s", msg); */
#define DEBUG_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_debug, NULL, 0, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: RT_DEBUG_TLOG(0, "%s", msg); */
#define RT_DEBUG_TLOG(rt, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_debug, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return rt; \
	} while (0)

/* eg: RTV_DEBUG_TLOG("%s", msg); */
#define RTV_DEBUG_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_debug, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return ; \
	} while (0)

/* eg: DEBUG_TLOG_EVERY_N(100, "%s", msg); */
#define DEBUG_TLOG_EVERY_N(n, fmt, args...) \
	do { \
		TLOG_EVERY_N(tlog_lvl_debug, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: DEBUG_TLOG_FIRST_N(10, "%s", msg); */
#define DEBUG_TLOG_FIRST_N(n, fmt, args...) \
	do { \
		TLOG_FIRST_N(tlog_lvl_debug, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: DEBUG_TLOG_IF(!(uid > 50000), "%s", msg); */
#define DEBUG_TLOG_IF(cond, fmt, args...) \
	do { \
		TLOG_IF(tlog_lvl_debug, cond, logger_flag_file, fmt"\n", ##args); \
	} while (0)



/* ========================================================================== */
/* TRACE_TLOG */
/* ========================================================================== */

/* eg: TRACE_TLOG("%s", msg); */
#define TRACE_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_trace, NULL, 0, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: RT_TRACE_TLOG(0, "%s", msg); */
#define RT_TRACE_TLOG(rt, fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_trace, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return rt; \
	} while (0)

/* eg: RTV_TRACE_TLOG("%s", msg); */
#define RTV_TRACE_TLOG(fmt, args...) \
	do { \
		write_to_logger(tlog_lvl_trace, NULL, 0, logger_flag_file, fmt"\n", ##args); \
		return ; \
	} while (0)

/* eg: TRACE_TLOG_EVERY_N(100, "%s", msg); */
#define TRACE_TLOG_EVERY_N(n, fmt, args...) \
	do { \
		TLOG_EVERY_N(tlog_lvl_trace, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: TRACE_TLOG_FIRST_N(10, "%s", msg); */
#define TRACE_TLOG_FIRST_N(n, fmt, args...) \
	do { \
		TLOG_FIRST_N(tlog_lvl_trace, n, logger_flag_file, fmt"\n", ##args); \
	} while (0)

/* eg: TRACE_TLOG_IF(!(uid > 50000), "%s", msg); */
#define TRACE_TLOG_IF(cond, fmt, args...) \
	do { \
		TLOG_IF(tlog_lvl_trace, cond, logger_flag_file, fmt"\n", ##args); \
	} while (0)



/* ========================================================================== */
/* CTRL_INTERFACES */
/* ========================================================================== */
#define SET_LOG_LEVEL(lvl) \
	do { \
		if ((lvl >= tlog_lvl_min) && (lvl < tlog_lvl_max)) { \
			ctrl_cfg->log_level = lvl; \
		} \
	} while (0)

#define SET_DAILY_MAX_FILES(max) \
	do { \
		if (max >= 0 && max <= DAILY_MAX_FILES) { \
			ctrl_cfg->daily_max_files = max; \
		} \
	} while (0)

#define SET_MAX_ONE_SIZE(max) \
	do { \
		if (max >= MIN_ONE_LOG_SIZE && max <= MAX_ONE_LOG_SIZE) { \
			ctrl_cfg->max_one_size = max; \
		} \
	} while (0)

#define SET_STOP_IF_DISKFULL(onoff) \
	do { \
		if (onoff == 0 || onoff == 1) { \
			ctrl_cfg->stop_if_diskfull = onoff; \
		} \
	} while (0)

#define SET_TIME_SLICE_SECS(secs) \
	do { \
		if (secs >= 0) { \
			ctrl_cfg->time_slice_secs = secs; \
		} \
	} while (0)





# endif // __TLOG_MACROS_H__
