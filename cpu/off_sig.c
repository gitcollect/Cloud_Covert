#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#define BIGNUM 10000000000
#define NUM_INT 1

#define CLUSTER_NR 3
#define DATA_NR 793

#define uint64_abs(a, b) (a>b? a-b : b-a)

uint64_t find_min (uint64_t *array, int array_number) {
	int x = 0;
	uint64_t min = array[x];
	while (x<array_number) {
		if (min > array[x])
			min = array[x];
    		x++;
  	}
	return min;
}

uint64_t find_max (uint64_t *array, int array_number) {
	int x = 0;
	uint64_t max = array[x];
	while (x<array_number) {
		if (max < array[x])
			max = array[x];
		x++;
	}
	return max;
}

int classify(uint64_t *array, int array_number, int cluster_number, int *bestCent) {
	int i, j, k;
	uint64_t mean, max, min;
	uint64_t *cent;

	for(i=0; i<array_number; i++)
		mean+=array[i];
	mean/=array_number;

	min = find_min(array, array_number);
	max = find_max(array, array_number);

	cent=(uint64_t *)calloc(cluster_number,sizeof(uint64_t));
	for (i=0; i<cluster_number; i++)
		cent[i] = min + (max-min)/(cluster_number-1) * i;
	
	uint64_t rMin;
	uint64_t dist;
	uint64_t cent_r;
	int tot;
	for (i=0; i<NUM_INT; i++) {
		for (j=0; j<array_number; j++) {
			rMin=BIGNUM;
			for (k=0; k<cluster_number; k++) {
				dist = uint64_abs(array[j],cent[k]);
				if (dist < rMin) {
					bestCent[j] = k;
					rMin=dist;
				}
			}
		}

		for (k=0; k<cluster_number; k++) {
			tot = 0;
			cent_r = 0;
			for (j=0; j<array_number; j++) {
				if (bestCent[j] == k) {
					cent_r += array[j];
					tot++;
				}
			}
			if (tot > 0) {
				cent[k] = cent_r/tot;
			}
			else
				cent[k] = min + random()%(max-min);
		}
	}

	for (i=0; i<cluster_number; i++)
		printf("cent[%d]: %ld\n",i, cent[i]);

	return 0;
}


int main(int argc, char *argv[]) {
	FILE *data_file = fopen("data.txt", "r");
	if (!data_file) {
		printf("file open error\n");
		return 0;
	}

	uint64_t data_array[DATA_NR];
	int i;
	int bestCent[DATA_NR] ;

	for (i=0; i<DATA_NR; i++) 
		fscanf(data_file, "%ld\n", &data_array[i]);


	classify(data_array, DATA_NR, CLUSTER_NR, bestCent);

	for (i=0; i<DATA_NR; i++) {
		if (bestCent[i] == 1)
			printf("0 ");
		if (bestCent[i] == 2)
			printf("1 ");
	}
	printf("\n");
	fclose(data_file);
	return 0;
}
