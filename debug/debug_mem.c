#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "debug_mem.h"

/**
 * @brief 获取当前进程的实际占用物理内存消耗 (不包含swap出去的)
 * @params 有效 pid, 获取改PID进程占用的物理内存, 若给 PID_SELF, 则给出自己消耗的内存
 *
 * @return: -1: failed, >=0: vmrss_size (单位: KB)
 */
int get_cur_vmrss_size(pid_t pid)
{
	int vmrss_size = -1;

	char *filename = malloc(STATUS_FILELEN);
	if (filename == NULL) return -1;

	if (pid == PID_SELF) {
		snprintf(filename, STATUS_FILELEN, FILE_PROC_SELF_STATUS);
	} else {
		snprintf(filename, STATUS_FILELEN, "/proc/%d/status", pid);
	}
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("fopen: ");
		free(filename);
		return -1;
	}
	free(filename);

	ssize_t n;
	size_t len = 0;
	char *line = NULL;
	while ((n = getline(&line, &len, fp)) != -1) {
		if (strncmp(line, "VmRSS", 5)) {
			continue;
		}
		int j;
		char *str, *token, *saveptr;
		for (j = 1, str = line; ; j++, str = NULL) {
			token = strtok_r(str, " ", &saveptr);
			if (token == NULL)
				break;
			if (j == 2) {
				vmrss_size = atoi(token);
				break;
			}
		}
	}
	if (line) { free(line); }

	fclose(fp);

	return vmrss_size;
}
