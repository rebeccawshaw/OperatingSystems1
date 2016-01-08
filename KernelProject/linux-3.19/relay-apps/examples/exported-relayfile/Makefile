DIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
KDIR   := /usr/src/linux-headers-3.13.0-32/

obj-m := exported-percpu-mod.o exported-global-mod.o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	$(CC) -o exported-percpu exported-percpu.c -lpthread
	$(CC) -o exported-global exported-global.c -lpthread

clean:
	/bin/rm -rf *.o *.ko *~ core *.mod.c .*.cmd .tmp_versions Module* exported-percpu exported-global

