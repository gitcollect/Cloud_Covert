#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>

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

int main(int argc, char *argv[]) {
	uint64_t tsc1, tsc2;
	while (1) {
		tsc1 = rdtsc() + 3000000;
		while (rdtsc()< tsc1 );
		printf("%lu\n", rdtsc()-tsc1);
	}

	return 0;
}
