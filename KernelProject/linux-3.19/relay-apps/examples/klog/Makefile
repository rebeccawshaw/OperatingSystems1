DIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
KDIR   := /usr/src/linux

obj-m := klog-mod.o test-mod.o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	$(CC) -o klog klog.c -lpthread

clean:
	/bin/rm -rf *.o *.ko *~ core *.mod.c .*.cmd .tmp_versions Module* klog

