DIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
KDIR   := /usr/src/linux

obj-m := read-mod.o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	$(CC) -o read read.c -lpthread

clean:
	/bin/rm -rf *.o *.ko *~ core *.mod.c .*.cmd .tmp_versions Module* read

