# SPDX-License-Identifier: GPL-2.0
# Out-of-tree build helper:
#   make
#   sudo insmod lk2.ko
#   cat /dev/keylogs
#   sudo rmmod lk2

obj-m += lk2.o
lk2-objs := lk2_main.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
