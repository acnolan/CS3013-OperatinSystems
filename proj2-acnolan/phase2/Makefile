obj-m := phase2_module.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -o phase2_test phase2_test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm phase2_test
	rm phase2_module.o.ur-safe
