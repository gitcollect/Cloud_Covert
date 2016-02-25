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

#define SIZE 256
#define LINE_SIZE 64

#define INTERVAL 1500000
#define ACCESS_TIME 100000
#define BIT_NR 100
#define CLOCK_NR BIT_NR*(INTERVAL/ACCESS_TIME)

uint8_t *mem_chunk;
uint64_t mem_size; 

int bank_index[6];
int *index_array;

uint64_t timing[CLOCK_NR];

void cache_flush(uint8_t *address) {
        __asm__ volatile("clflush (%0)": :"r"(address) :"memory");
        return;
}

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

void receiver() {
	volatile uint8_t next = 0;;
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(1, &set);
        if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
                fprintf(stderr, "Error set affinity\n")  ;
                return;
        }

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


	printf("Receiving......\n");
	uint64_t tsc, tsc1, access_nr;
	for (j=0; j<CLOCK_NR; j++) {
		access_nr = 0;
		tsc1 = rdtsc() + ACCESS_TIME;
		while (rdtsc() < tsc1) {
			access_nr ++;
			for (i=0; i<SIZE/LINE_SIZE; i++) {
				next += mem_chunk[index_array[i]];
				cache_flush(&mem_chunk[index_array[i]]);
			}
		}
		timing[j] = ACCESS_TIME/access_nr;
	}
	return;
}

int main(int argc, char* argv[]) {
	mem_size = 1024*1024*1024;
	int fd = open("/mnt/hugepages/nebula2", O_CREAT|O_RDWR, 0755);
	
	if (fd < 0) {
		printf("file open error!\n");
		return 0;
	}

	mem_chunk = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem_chunk == MAP_FAILED) {
		printf("map error!\n");
		unlink("/mnt/hugepages/nebula2");
		return 0;
	}

	int i;
	for (i=0; i<mem_size/sizeof(uint8_t); i++)
		mem_chunk[i] = 1;

	receiver();

	for (i=0; i<CLOCK_NR; i++)
		printf("%lu\n", timing[i]);
	printf("\n");

	munmap(mem_chunk, mem_size);
	return 0;

}
