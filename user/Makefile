KERNEL_INCLUDE ?= /lib/modules/`uname -r`/build/include

test:
	gcc -o test -isystem $(KERNEL_INCLUDE) test.c

clean:
	rm -f test
