obj-m := phase1_module.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: phase1_module

phase1_module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -o phase1_test phase1_test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm phase1_module.o.ur-safe
	rm phase1_test
