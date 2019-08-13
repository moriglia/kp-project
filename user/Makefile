KERNEL_INCLUDE ?= /lib/modules/`uname -r`/build/include

all: test test2

test:
	gcc -o test -isystem $(KERNEL_INCLUDE) test.c

test2:
	gcc -o test2 -isystem $(KERNEL_INCLUDE) test2.c

clean:
	rm -f test test2
