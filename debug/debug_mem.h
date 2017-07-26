#ifndef __DEBUG_MEM_H__
#define __DEBUG_MEM_H__


#define		PID_SELF				(-1)
#define		STATUS_FILELEN			(32)
#define		FILE_PROC_SELF_STATUS	"/proc/self/status"

/**
 * @brief 获取当前进程的实际占用物理内存消耗 (不包含swap出去的)
 * @params 有效 pid, 获取改PID进程占用的物理内存, 若给 PID_SELF, 则给出自己消耗的内存
 *
 * @return: -1: failed, >=0: vmrss_size (单位: KB)
 */
int get_cur_vmrss_size(pid_t pid);


#endif //__DEBUG_MEM_H__
