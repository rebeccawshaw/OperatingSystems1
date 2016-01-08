DIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
KDIR   := /usr/src/linux

obj-m := readtest-mod.o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	$(CC) -D_GNU_SOURCE -o readtest readtest.c -lpthread

clean:
	/bin/rm -rf *.o *.ko *~ core *.mod.c .*.cmd .tmp_versions Module* readtest

