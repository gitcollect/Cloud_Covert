KDIR := /lib/modules/$(shell uname -r)/build
CC = gcc

obj-m += covert_channel.o

all: kernel user

kernel:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

user:
	$(CC) -o test test.c
	$(CC) -o recev_sig recev_sig.c
	$(CC) -o off_sig off_sig.c

clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp* test recev_sig off_sig
