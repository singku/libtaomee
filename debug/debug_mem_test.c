/*
 * 测试 get_cur_vmrss_size 接口获得自己当前使用的实际物理内存
 * compile: gcc -o /tmp/debug_mem_test debug_mem_test.c -ltaomee -I/usr/include/libtaomee/
 * run: /tmp/debug_mem_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <debug/debug_mem.h>


int main(int argc, char **argv)
{
	char *p = malloc(10 * 1024 * 1024);
	memset(p, 0, 10*1024*1024);

	int x = 0;
	int usec = 100000;
	char *q;
	while (1) {
		printf("vmrss_size: %d KB\n", get_cur_vmrss_size(PID_SELF));
		usleep(usec);

		++x;
		if (x == 20) {
			q = malloc(10 * 1024 * 1024);
		}

		if (x == 40) {
			if (q) {
				memset(q, 0, 10*1024*1024);
			}
			usec = 1000000;
		}
	}

	return 0;
}
