obj-m += testing.o
CONFIG_MODULE_SIG=n

.PHONY: c
c: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules