/**
 *============================================================
 *  @file      log.h
 *  @brief     用于记录日志，一共分9种日志等级。日志文件可以自动轮转。注意：如果使用自动轮转，\n
 *             必须保证每天写的日志文件个数不能超过log_init时设定的最大文件个数，否则日志会写乱掉。
 *             必须先调用log_init/log_init_t来初始化日志功能。注意，每条日志不能超过8000字节。\n
 *             如果编译程序时定义宏LOG_USE_SYSLOG，则会利用syslog来记录日志，使用的facility是LOG_USER。
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_LOG_H_
#define LIBTAOMEE_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef  likely
#undef  likely
#endif
#define likely(x) __builtin_expect(!!(x), 1)

#ifdef  unlikely
#undef  unlikely
#endif
#define unlikely(x) __builtin_expect(!!(x), 0)

#ifdef USE_TLOG

#include <libtaomee/log_old.h>
#include <libtaomee/tlog/tlog.h>

#ifdef EMERG_LOG
#undef EMERG_LOG
#endif
#define EMERG_LOG(fmt, args...) FATAL_TLOG(fmt, ##args)

#ifdef KEMERG_LOG
#undef KEMERG_LOG
#endif
#define KEMERG_LOG(key, fmt, args...) FATAL_TLOG("%u "fmt, key, ##args)

#ifdef ALERT_LOG
#undef ALERT_LOG
#endif
#define ALERT_LOG(fmt, args...) FATAL_TLOG(fmt, ##args)

#ifdef KALERT_LOG
#undef KALERT_LOG
#endif
#define KALERT_LOG(key, fmt, args...) FATAL_TLOG("%u "fmt, key, ##args)

#ifdef CRIT_LOG
#undef CRIT_LOG
#endif
#define CRIT_LOG(fmt, args...) FATAL_TLOG(fmt, ##args)

#ifdef KCRIT_LOG
#undef KCRIT_LOG
#endif
#define KCRIT_LOG(key, fmt, args...) FATAL_TLOG("%u "fmt, key, ##args)

#ifdef ERROR_LOG
#undef ERROR_LOG
#endif
#define ERROR_LOG(fmt, args...) ERROR_TLOG(fmt, ##args)

#ifdef KERROR_LOG
#undef KERROR_LOG
#endif
#define KERROR_LOG(key, fmt, args...) ERROR_TLOG("%u "fmt, key, ##args)

#ifdef WARN_LOG
#undef WARN_LOG
#endif
#define WARN_LOG(fmt, args...) WARN_TLOG(fmt, ##args)

#ifdef KWARN_LOG
#undef KWARN_LOG
#endif
#define KWARN_LOG(key, fmt, args...) WARN_TLOG("%u "fmt, key, ##args)

#ifdef NOTI_LOG
#undef NOTI_LOG
#endif
#define NOTI_LOG(fmt, args...) INFO_TLOG(fmt, ##args)

#ifdef KNOTI_LOG
#undef KNOTI_LOG
#endif
#define KNOTI_LOG(key, fmt, args...) INFO_TLOG("%u "fmt, key, ##args)

#ifdef INFO_LOG
#undef INFO_LOG
#endif
#define INFO_LOG(fmt, args...) INFO_TLOG(fmt, ##args)

#ifdef KINFO_LOG
#undef KINFO_LOG
#endif
#define KINFO_LOG(key, fmt, args...) INFO_TLOG("%u "fmt, key, ##args)

#ifdef DEBUG_LOG
#undef DEBUG_LOG
#endif
#define DEBUG_LOG(fmt, args...) DEBUG_TLOG(fmt, ##args)

#ifdef KDEBUG_LOG
#undef KDEBUG_LOG
#endif
#define KDEBUG_LOG(key, fmt, args...) DEBUG_TLOG("%u "fmt, key, ##args)

#ifdef TRACE_LOG
#undef TRACE_LOG
#endif
#define TRACE_LOG(fmt, args...) TRACE_TLOG(fmt, ##args)

#ifdef KTRACE_LOG
#undef KTRACE_LOG
#endif
#define KTRACE_LOG(key, fmt, args...) TRACE_TLOG("%u "fmt, key, ##args)

#ifdef BOOT_LOG
#undef BOOT_LOG
#endif
#define BOOT_LOG(OK, fmt, args...) RT_BOOT_TLOG(OK, fmt, ##args)


#ifdef BOOT_LOG2
#undef BOOT_LOG2
#endif
#define BOOT_LOG2(OK, n, fmt, args...) RT_BOOT_TLOG(OK, fmt, ##args)

#if 0
#ifdef ERROR_RETURN
#undef ERROR_RETURN
#define ERROR_RETURN((fmt, args...), rt) RT_ERROR_TLOG(rt, fmt, ##args)
#endif

#ifdef ERROR_RETURN_VOID
#undef ERROR_RETURN_VOID
#define ERROR_RETURN_VOID(fmt, args...) RTV_ERROR_TLOG(fmt, ##args)
#endif

#ifdef WARN_RETURN
#undef WARN_RETURN
#define WARN_RETURN((fmt, args...), rt) RT_WARN_TLOG(rt, fmt, ##args)
#endif

#ifdef WARN_RETURN_VOID
#undef WARN_RETURN_VOID
#define WARN_RETURN_V0ID(fmt, args...) RTV_WARN_TLOG(fmt, ##args)
#endif

#ifdef DEBUG_RETURN
#undef DEBUG_RETURN
#define DEBUG_RETURN((fmt, args...), rt) RT_DEBUG_TLOG(rt, fmt, ##args)
#endif

#ifdef DEBUG_RETURN_VOID
#undef DEBUG_RETURN_VOID
#define DEBUG_RETURN_VOID(fmt, args...) RTV_DEBUG_TLOG(fmt, ##args)
#endif
#endif

/**
 * @def ERROR_RETURN
 * @brief 输出log_lvl_error等级的日志，并且返回Y到上一级函数。\n
 *        用法示例：ERROR_RETURN(("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno)), -1);
 */
#define ERROR_RETURN(X, Y) \
		do { \
			ERROR_LOG X; \
			return Y; \
		} while (0)

/**
 * @def ERROR_RETURN_VOID
 * @brief 输出log_lvl_error等级的日志，并且返回上一级函数。\n
 *        用法示例：ERROR_RETURN("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno));
 */
#define ERROR_RETURN_VOID(fmt, args...) \
		do { \
			ERROR_LOG(fmt, ##args); \
			return; \
		} while (0)

/**
 * @def WARN_RETURN
 * @brief 输出log_lvl_warning等级的日志，并且返回ret_到上一级函数。\n
 *        用法示例：WARN_RETURN(("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno)), -1);
 */
#define WARN_RETURN(msg_, ret_) \
		do { \
			WARN_LOG msg_; \
			return (ret_); \
		} while (0)

/**
 * @def WARN_RETURN_VOID
 * @brief 输出log_lvl_warning等级的日志，并且返回上一级函数。\n
 *        用法示例：WARN_RETURN_VOID("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno));
 */
#define WARN_RETURN_VOID(fmt, args...) \
		do { \
			WARN_LOG(fmt, ##args); \
			return; \
		} while (0)

/**
 * @def DEBUG_RETURN
 * @brief 输出log_lvl_debug等级的日志，并且返回ret_到上一级函数。\n
 *        用法示例：DEBUG_RETURN(("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno)), -1);
 */
#define DEBUG_RETURN(msg_, ret_) \
		do { \
			DEBUG_LOG msg_; \
			return (ret_); \
		} while (0)

/**
 * @def DEBUG_RETURN_VOID
 * @brief 输出log_lvl_debug等级的日志，并且返回上一级函数。\n
 *        用法示例：DEBUG_RETURN_VOID("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno));
 */
#define DEBUG_RETURN_VOID(fmt, args...) \
		do { \
			DEBUG_LOG(fmt, ##args); \
			return; \
		} while (0)

#else
#include <libtaomee/log_old.h>
#endif  //USE_TLOG

#ifdef __cplusplus
}
#endif

#endif  //LIBTAOMEE_LOG_H

