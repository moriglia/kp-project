KERNEL_DIR ?= /lib/modules/`uname -r`/build

all: modulebuild userbuild

clean: moduleclean userclean

modulebuild:
	make -f ./module/Makefile KERNEL_DIR=$(KERNEL_DIR)

moduleclean:
	make clean -f ./module/Makefile

userbuild:
	make -f ./user/Makefile

userclean:
	make clean -f ./user/Makefile

