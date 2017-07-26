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
#include <stdlib.h>
#include <time.h>

#include <unistd.h>

#include "shmq.h"

int main(int argc, char* argv[])
{
	shm_cq_t* cq = shm_cq_create("shmcq_test", 1000, 600);
	FILE* fp = fopen(argv[1], "r");
	char buf[600];
	srand(time(0));
	int n = rand() % 600 + 1;
	int rd;
	printf("%d %d %d\n\n", n, cq->head, cq->tail);
	while ( (rd = fread(buf, 1, n, fp)) == n) {
		while (shm_cq_push(cq, buf, n));
		printf("%d %d %d\n", n, cq->head, cq->tail);
		n = rand() % 600 + 1;
	}
	if (rd) {
		while (shm_cq_push(cq, buf, rd));
		printf("%d %d %d\n", rd, cq->head, cq->tail);
	}
	shm_cq_destroy(cq);
}
