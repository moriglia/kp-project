KERNEL_INCLUDE ?= /lib/modules/`uname -r`/build/include

apps=$(patsubst %.c,%,$(shell ls *.c))


all: $(apps)

%: %.c
	gcc -o $@ -isystem $(KERNEL_INCLUDE) $(shell if grep -q pthread.h $< ; then echo -pthread ; fi) $< 


clean:
	rm -f $(apps)
