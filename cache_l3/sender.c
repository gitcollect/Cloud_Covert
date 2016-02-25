#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#define ASSOC 20
#define SLICES 6
#define WAY_SIZE 131072
#define LINE_SIZE 64
#define SET_NUM (WAY_SIZE/LINE_SIZE)

#define INTERVAL 1500000

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

char *buf;
char **head;

int conflict_sets[SLICES][ASSOC];  // Records the conflicted set indexes.

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

void initialize() {
	char **ptr1, **ptr2;
	int i, j, k;
	char *tmp;
	int idx1, idx2;

	for (j=0; j<SLICES; j++) {
		for (k=0; k<ASSOC; k++) {
			idx1 = conflict_sets[j][k];
			ptr1 = (char **)&buf[idx1*WAY_SIZE];
			*ptr1 = (char *)ptr1;
		}

		for (k=ASSOC-1; k>=1; k--){
			idx1 = conflict_sets[j][k];
			idx2 = conflict_sets[j][k-1];
			ptr1 = (char**)&buf[idx1*WAY_SIZE];
			ptr2 = (char**)&buf[idx2*WAY_SIZE];
			tmp = *ptr1;
			*ptr1 = *ptr2;
			*ptr2 = tmp;
		}

		for (k=0; k<ASSOC; k++){
			idx1 = conflict_sets[j][k];
			ptr1 = (char **)&buf[idx1*WAY_SIZE];
			ptr2 = (char **)*ptr1;
			*(ptr2+1) = (char*)(ptr1+1);
		}

		idx1 = conflict_sets[j][0];
		head[j] = &buf[idx1*WAY_SIZE];
	}
} 


void send() {
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(0, &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n")  ;
		return;
	}
	int i;
	printf("Sending...\n");
	uint64_t tsc;
	int index = -1;

	while(1) {
		tsc = rdtsc() + INTERVAL;
		index = (index+1)%10;
		if (message[index] == 1) {
			while(rdtsc() < tsc) {
				for (i=0; i<SLICES; i++) {
					__asm__("mov %0,%%r8\n\t"
						"mov %%r8,%%rsi\n\t"
						"xor %%eax, %%eax\n\t"
						"loop: mov (%%r8), %%r8\n\t"
						"cmp %%r8,%%rsi\n\t"
						"jne loop\n\t"
						"xor %%eax, %%eax\n\t"
						:
						:"r"(head[i])
						:"esi","r8","eax");
				}
			}
		}
		if (message[index] == 0) {
			while(rdtsc() < tsc);
		}
	}
}

int main (int argc, char *argv[]) {
        int i, j, k;
        FILE *set_file = fopen("sender_conflict_sets", "r");
	
        if (!set_file) {
                printf("error\n");
		return 0;
	}

        for (j=0; j<SLICES; j++) {
		fseek(set_file, 0, SEEK_SET);
		for (k=0; k<j; k++)
			fscanf(set_file, "%*[^\n]\n", NULL);
		for (k=0; k<ASSOC; k++)
			fscanf(set_file, "%d", &conflict_sets[j][k]);
	}
	
        fclose(set_file);

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

	head = (char **)calloc(SLICES, sizeof(char *));

	initialize();
	MsgInit();
	
	send();
	
	munmap(buf, buf_size);
}
