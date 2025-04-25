# Set the kernel build directory
KERNELDIR := /lib/modules/$(shell uname -r)/build

# Set the current working directory for the module
PWD := $(shell pwd)

# Module name (adjust if your module name is different)
obj-m := kdeque.o

# Default target
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

# Install target
install:
	sudo cp kdeque.ko /lib/modules/$(shell uname -r)/kernel/drivers/misc/
	sudo depmod -a

# Clean up generated files
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
