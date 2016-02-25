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

#define LINE_SIZE 64
#define INTERVAL 1500000
#define ACCESS_TIME 100000
#define BIT_NR 100

#define CLOCK_NR BIT_NR*(INTERVAL/ACCESS_TIME)

uint8_t *buf;
uint8_t *head;
uint64_t timing[CLOCK_NR];

void receiver() {
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(1, &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n")  ;
		return;
	}
	printf("Begin Receiving......\n");

	uint64_t tsc;
	uint64_t i, access_nr;

	int *read_address;
	read_address = (int *)(head+LINE_SIZE-1);
        int value = 0x0;
	for (i=0; i<CLOCK_NR; i++) {
		access_nr = 0;
		tsc = rdtsc() + ACCESS_TIME;
		while (rdtsc() < tsc) {
			 __asm__(
                              "lock; xaddl %%eax, %1\n\t"
                              :"=a"(value)
                              :"m"(*read_address), "a"(value)
                              :"memory");
			access_nr ++;
		}
		timing[i] = ACCESS_TIME/access_nr;
	}
}

int main (int argc, char *argv[]) {
        uint64_t buf_size = 1024*1024*1024;
        int fd = open("/mnt/hugepages/nebula2", O_CREAT|O_RDWR, 0755);
        if (fd<0) {
                printf("file open error!\n");
                return 0;
        }

        buf = mmap(0, buf_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (buf == MAP_FAILED) {
                printf("map error!\n");
                unlink("/mnt/hugepages/nebula2");
                return 0;
        }

	int i;
	for (i=0; i<buf_size/sizeof(uint8_t); i++)
		buf[i] = 1;
	head = &buf[0];

	receiver();

	for (i=0; i<CLOCK_NR; i++)
		printf("%lu ", timing[i]);

	printf("\n");
	
	munmap(buf, buf_size);
}
