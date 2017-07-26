#ifndef __TLOG_H__
#define __TLOG_H__


#include "tlog_macros.h"
#include "tlog_decl.h"



#define INIT_DEFAULT_LOGGER_SYSTEM(_logdir, _pfx, _svc, _gameid, _svrtp) \
	do { \
		int _ret; \
		_ret = init_logger_ctrl_cfg(def_log_level, def_daily_max_files, \
				def_max_one_size, def_stop_if_diskfull, def_time_slice_secs); \
		if (_ret < 0) { fprintf(stderr, "Failed to init ctrl_cfg\n"); exit(-1); } \
		_ret = init_looger_svc_info(_svc, _gameid, _svrtp); \
		if (_ret < 0) { fprintf(stderr, "Failed to init svc_info\n"); exit(-1); } \
		_ret = init_logger(_logdir, _pfx); \
		if (_ret < 0) { fprintf(stderr, "Failed to init_logger\n"); exit(-1); } \
	} while (0)


/**
 * tlog interface
 */
int init_logger_ctrl_cfg(int lvl, int daily_max_files, size_t max_one_size,
		int stop_if_diskfull, int time_slice_secs);
int init_looger_svc_info(const char *svcname, int gameid, const char *svrtype);
int init_logger(const char *dir, const char *prefix);

void write_to_logger(int lvl, const char *logtype,
		uint32_t uid, uint32_t flag, const char *fmt, ...);

void boot_tlog(int ok, int dummy, const char *fmt, ...);
void std_tlog(int ok, const char *fmt, ...);


/** helper functions */
int is_traced_uid(uint32_t uid);
int gen_timestring(char *buf, int maxlen, logfile_t *logfile, char *datedir);
void tlog_get_host_name(char *hostname, int maxlen);
void tlog_my_user_name(char *username, int maxlen);

#endif // __TLOG_H__
