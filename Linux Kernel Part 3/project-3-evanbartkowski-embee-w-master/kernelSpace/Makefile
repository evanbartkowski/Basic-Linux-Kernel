ccflags-y := -std=gnu99 -Wno-declaration-after-statement
obj-m += filesystem.o

filesystem-objs := kernel_filesystem.o dynamicmemorybank.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
