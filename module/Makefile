KERNEL_DIR ?= /lib/modules/`uname -r`/build

obj-m = kp.o
kp-objs = device.o module.o periodic.o

all: 
	make -C $(KERNEL_DIR) M=`pwd` modules

clean:
	make -C $(KERNEL_DIR) M=`pwd` clean

