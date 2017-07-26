/**
 * =====================================================================================
 *
 * @file  a.c
 *
 * @brief 
 *
 * compiler  GCC4.1.2
 * platform  Linux
 *
 * copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 * 		
 * ------------------------------------------------------------
 * 	note: set tabspace=4 
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "shmq.h"

int main()
{
	sleep(1);
	shm_cq_t* cq = shm_cq_attach("shmcq_test");
	FILE* fp = fopen("tt", "w");
	for (;;) {
		char* a;
		uint32_t size = shm_cq_pop(cq, (void**)&a);
		if (size) {
			fwrite(a, 1, size, fp);
			fflush(0);
		}
		//printf("%d %d %d\n", size, cq->head, cq->tail);
	}

}
