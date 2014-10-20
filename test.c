#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_CacheClean 188

int main(int argc, char* argv[]) {
	int t = atoi(argv[1]);
	syscall(__NR_CacheClean, t);
	printf("Message Transmitted!\n");
	return 0;
}
