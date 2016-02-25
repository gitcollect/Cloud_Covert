#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#ifdef __i386
__inline__ uint64_t rdtsc(void) {
        uint64_t x;
        __asm__ volatile ("rdtsc" : "=A" (x));
        return x;
}
#elif __amd64
__inline__ uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}
#endif

void cache_flush(uint8_t *address) {
        __asm__ volatile("clflush (%0)": :"r"(address) :"memory");
        return;
}

#define LINE_SIZE 64
#define INTERVAL 1500000

int message[10];
char *buf;
char *head;

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

void send() {
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(0, &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n")  ;
		return;
	}
	printf("Begin Sending......\n");

	int *read_address;
	int value = 0x0;
	int index = -1;
	uint64_t tsc;

	while(1) {
		tsc = rdtsc() + INTERVAL;
		index = (index+1)%10;
		read_address = (int *)(head+LINE_SIZE-message[index]);
		if (message[index] == 1) {
			while(rdtsc() < tsc) {
				__asm__(
       			              "lock; xaddl %%eax, %1\n\t"
               			      :"=a"(value)
	                	      :"m"(*read_address), "a"(value)
	       		              :"memory");
			}
		}
		else {
			while(rdtsc() < tsc) {
				value += head[0];
				cache_flush(&head[0]);
			}
		}
	}
}

int main (int argc, char *argv[]) {
        uint64_t buf_size = 1024*1024*1024;
        int fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
        if (fd<0) {
                printf("file open error!\n");
                return 0;
        }

        buf = mmap(0, buf_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (buf == MAP_FAILED) {
                printf("map error!\n");
                unlink("/mnt/hugepages/nebula1");
                return 0;
        }
	head = &buf[0];
	MsgInit();

	send();
	
	munmap(buf, buf_size);
}
