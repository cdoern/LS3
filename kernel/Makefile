obj-m += ls3.o
CONFIG_MODULE_SIG=n

.PHONY: modules
modules: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

.PHONY: clean
clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
