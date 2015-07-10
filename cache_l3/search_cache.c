#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#define ASSOC 20
#define SLICES 6
#define THRESHOLD 3000

#define WAY_SIZE 131072
#define LINE_SIZE 64

#define INIT_CONFLICT_SETS 110
#define MAX_CONFLICT_SETS 200

#define TRIALS 10000

#define MISS_NR 21

char* head;
char* buf;
int conflict_sets[MAX_CONFLICT_SETS];
static int cur_slice=0;
int pattern_seen;

int slice_nr[SLICES];

int set;

void initialize(int __set) {
	int j, k;
	char **ptr1, **ptr2;
	char *tmp;

	for (j=0; j<INIT_CONFLICT_SETS; j++){
		ptr1 = (char **)&buf[__set*LINE_SIZE+j*WAY_SIZE];
		*ptr1 = (char*)ptr1;
       }

	for (j=INIT_CONFLICT_SETS-1; j>=1; j--){
		k = rand()%j;
		ptr1 = (char **)&buf[__set*LINE_SIZE+j*WAY_SIZE];
		ptr2 = (char **)&buf[__set*LINE_SIZE+k*WAY_SIZE];
		tmp = *ptr1;
		*ptr1 = *ptr2;
		*ptr2 = tmp;
	}
	head = &buf[__set*LINE_SIZE+j*WAY_SIZE];
} 

void initConflictSets() {
	int i;
	for (i=0; i<MAX_CONFLICT_SETS; i++)
		conflict_sets[i] = -1;
}


int getSize(){
	int i=0; 
	int line;
	char *ptr = head;
	do {
		line = (ptr-buf)/(WAY_SIZE);
		ptr = *(char **)ptr;
		i++;
	} while (ptr != head);
	return i;
} 

void add(int line) {
	int size = getSize();
	int pos = rand()%size;
	char *prev = head;
	char *cur, *next;
	cur = &buf[line * WAY_SIZE + set * LINE_SIZE];

	int i;
	while (i<pos) {
		prev = *(char **)prev;
		i++;
	}

	next = *(char**)prev;
	*(char **)prev = cur;
	*(char **)cur = next;
}

void removeLine(int line)
{
	char **prev;
	char *cur, *next;
	prev = (char **)head;
	cur = &buf[line*WAY_SIZE+set*LINE_SIZE];

	while(*prev != cur)
		prev = (char **)(*prev);

	next = *(char **)cur;
	if(cur == head)
		head = next;
	*prev = next;
} 
   
int check(int cur_line)
{
	char **ptr = (char **)head;
	unsigned int * time_buf;
	int candidate[MISS_NR+5];
	int i, j, k;
	int line;
	int head_line;

	for (i=0; i<=MISS_NR+5; i++)
		candidate[i] = -1;
	i = 0;
	do {
		time_buf = (unsigned int *)(ptr+1);
		if ((*time_buf) > THRESHOLD) {
			if (i>MISS_NR) {
				break;
//				return -1;
			}
			line = ((char*)ptr - buf)/(WAY_SIZE);
			candidate[i++] = line;
		}
		ptr = (char **)(*ptr);
	} while(ptr != (char **)head);

// 	if (i>0)
//		printf("cur_line: %d, number of time exceeding threshold is %d\n", cur_line, i); 

	if (i==0)
		return 0;


	cur_slice = -1;
	if (i<MISS_NR) {
		pattern_seen++;
		if (pattern_seen == 3) {
			for (j=0; j<i; j++) {
				line = candidate[j];
				if (conflict_sets[line] != -1) {
					cur_slice = conflict_sets[line];
					break;
				}
			}
			if (cur_slice == -1) {
				for (k=0; k<SLICES; k++) {
					if (slice_nr[k] == 0) {
						cur_slice = k;
						break;
					}
				}
			}
			if (slice_nr[cur_slice] + i >= MISS_NR) {
				for (j=0; j<i; j++) {
					line = candidate[j];
					removeLine(line);
					conflict_sets[line] = cur_slice;
				}
				slice_nr[cur_slice] += i;
			}
			else {
				for (j=0; j<i; j++) {
					line = candidate[j];
					conflict_sets[line] = cur_slice;
				}
				line = candidate[i-1];
				removeLine(line);
				slice_nr[cur_slice] += 1;
			}
			return 0;
		}
		else
			return -1;
	}
	
	if (i>=MISS_NR){
		for (j=0; j<i; j++){
			line = candidate[j];
			if (conflict_sets[line] != -1) {
				cur_slice = conflict_sets[line];
				break;
			}
		}
		if (cur_slice == -1) {
			for (k=0; k<SLICES; k++) {
				if (slice_nr[k] == 0) {
					cur_slice = k;
					break;
				}
			}
		}

		for (j=0; j<i; j++) {
			line = candidate[j];
			removeLine(line);
			conflict_sets[line] = cur_slice;
		}
		slice_nr[cur_slice] += i;
		return 0;
	} 
}
 
int clearTimeBuffer()
{ 
	char **ptr = (char **)head;
	unsigned int * time_buf;
 
	do {
		time_buf = (unsigned int *)(ptr+1);
		*time_buf = 0;
		ptr = (char **)(*ptr);
	} while(ptr != (char **)head); 
}
       


void prime(char *ptr)
{
   __asm__("mov %0,%%r8\n\t"
           "mov %%r8,%%rsi\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
           "loop: mov (%%r8), %%r8\n\t"
           "cmp %%r8,%%rsi\n\t"
           "jne loop\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
           :
           :"r"(ptr)
           :"esi","r8","eax");
}

void probe(char *ptr)
{ 
   __asm__("mov %0,%%r8\n\t"
           "mov %%r8,%%rsi\n\t"
           "loop1: mov %%r8, %%r9\n\t"
           "xor %%eax, %%eax\n\t"
           "lfence\n\t"
           "rdtsc\n\t"
           "mov %%eax,%%edi\n\t"
           "mov (%%r8), %%r8\n\t"
           "xor %%eax, %%eax\n\t"
           "lfence\n\t"
           "rdtsc\n\t"
           "sub %%edi, %%eax\n\t"
           "cmp $200, %%eax\n\t"
           "jle loop2\n\t"
           "incl 8(%%r9)\n\t"
           "loop2:cmp %%r8,%%rsi\n\t"
           "jne loop1\n\t"
           :
           :"r"(ptr)
           :"eax","edx","esi","edi","r8","r9"
           );
} 


uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a"(a));
        return a;
}

int main (int argc, char *argv[]) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);

	FILE *output;
	int fd;

	uint64_t buf_size = 1024*1024*1024;

	set = 0;
	int role = atoi(argv[1]);
	if (role == 0) {
 		output= fopen("sender_conflict_sets", "at");
		if (!output) {
			printf("file open error!\n");
			return 0;
		}

		fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
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
	}
	else if (role == 1) {
 		output= fopen("receiver_conflict_sets", "at");
		if (!output) {
			printf("file open error!\n");
			return 0;
		}

		fd = open("/mnt/hugepages/nebula2", O_CREAT|O_RDWR, 0755);
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
	}
	else {
		printf("specify the sender or receiver\n");
		return 0;
	}



	initialize(set);
	initConflictSets();
	
	int count = INIT_CONFLICT_SETS;
	int i, done;
	int success = -1;
	int time_buf_start;
	int seed = 0;
	srand(seed);
	
	for (i=0; i<SLICES; i++) 
		slice_nr[i] = 0;

	while (count < MAX_CONFLICT_SETS) {
		add(count);
		pattern_seen=0;
		do{
			i = 0;
			clearTimeBuffer();
			while (i < TRIALS){
				prime(head);
				probe(head);
				i++;
			}
			success = check(count);
		} while(success != 0);

		done = 1;
		for (i=0; i<SLICES; i++) {
			if (slice_nr[i] < MISS_NR) {
				done = 0;
				break;
			}
		}
		if (done == 1)
			break;
		count++;
	}

	for (count=0; count<(SLICES); count++){
		i = 0;
		done = 0;
		while(done<MISS_NR){
			if (conflict_sets[i] == count){
				if ((done == MISS_NR-1))
					fprintf(output, "%d\n",i);
				else 
					fprintf(output, "%d ",i);
				done++;
			}
			i++;
		}
	}
	
	munmap(buf, buf_size);
	fclose(output);
}
