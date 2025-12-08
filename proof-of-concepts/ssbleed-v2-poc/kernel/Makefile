obj-m   := mdu_kernel.o
mdu_kernel-objs	:=	main.o asm.o
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean