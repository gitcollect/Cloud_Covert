#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define SIZE 1024
#define LINE_SIZE 64

#define INTERVAL 1500000

uint8_t *mem_chunk;
uint64_t mem_size; 

int bank_index[6];
int *index_array;

int message[10];

void MsgInit(void) {
        message[0] = 1;
        message[1] = 0;
        message[2] = 1;
        message[3] = 0;
        message[4] = 1;
        message[5] = 0;
        message[6] = 1;
        message[7] = 0;
        message[8] = 1;
        message[9] = 0;
}

void cache_flush(uint8_t *address) {
        __asm__ volatile("clflush (%0)": :"r"(address) :"memory");
        return;
}

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

void send() {
	volatile uint8_t next = 0;;

	int i, j;

	bank_index[0] = 6;
	bank_index[1] = 7;
	bank_index[2] = 15;
	bank_index[3] = 16;
	bank_index[4] = 20;
	bank_index[5] = 21;

	i = 63;
	int bank_mask = (((i>>5)&0x1)<<bank_index[5])/sizeof(uint8_t) +
			 (((i>>4)&0x1)<<bank_index[4])/sizeof(uint8_t) +
			 (((i>>3)&0x1)<<bank_index[3])/sizeof(uint8_t) +
			 (((i>>2)&0x1)<<bank_index[2])/sizeof(uint8_t) +
			 (((i>>1)&0x1)<<bank_index[1])/sizeof(uint8_t) +
			 (((i>>0)&0x1)<<bank_index[0])/sizeof(uint8_t);
	
	index_array = (int *)malloc(SIZE/LINE_SIZE*sizeof(int));

	int index_value;

	i = 0; 
	j = 0;
	while (j<SIZE/LINE_SIZE) {
		index_value = i * LINE_SIZE;
		if ((int)(index_value&bank_mask) == 0) {
			index_array[j] = index_value/sizeof(uint8_t);
			j++;
		}
		i++;
	}

	uint64_t tsc;
	int index;

	printf("Sending......\n");
	while (1) {
		tsc = rdtsc() + INTERVAL;
		index = (index+1)%10;
		if (message[index] == 1) {
			while(rdtsc() < tsc) {
				for (i=0; i<SIZE/LINE_SIZE; i++) {
					next += mem_chunk[index_array[i]];
					cache_flush(&mem_chunk[index_array[i]]);
				}
			}
		}
		if (message[index] == 0) {
			while(rdtsc() < tsc);
		}
	}
	return;
}

int main(int argc, char* argv[]) {
	mem_size = 1024*1024*1024;
	int fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
	
	if (fd < 0) {
		printf("file open error!\n");
		return 0;
	}

	mem_chunk = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem_chunk == MAP_FAILED) {
		printf("map error!\n");
		unlink("/mnt/hugepages/nebula1");
		return 0;
	}

	int i;
	for (i=0; i<mem_size/sizeof(uint8_t); i++)
		mem_chunk[i] = 1;

	MsgInit();

	send();

	munmap(mem_chunk, mem_size);
	return 0;

}
