#Makefile to build apc8620

# compiling isrxxx source files
ISRNAMES := isr408 isr1k110 isr400 isr409 isr482 isr330 isr340 isr470 isr480 isr560 isr570

ISRDIR      := ../isr/
ISRDIRNAMES := $(addprefix $(ISRDIR), $(ISRNAMES))
ISROBJS     := $(addsuffix .o, $(ISRDIRNAMES))
NAME    := dev8620
TARGET  := apc8620
MODULE  := $(TARGET).ko
OBJS    := $(addsuffix .o, $(NAME)) $(ISROBJS)
CPUTYPE := $(shell uname -p)

# compiling all source files
$(TARGET)-objs := $(OBJS)
ifneq ($(KERNELRELEASE),)
obj-m := $(TARGET).o
else
KDIR := /lib/modules/$(shell uname -r)/build #use this line for Fedora/Redhat/CentOS
#KDIR := /usr/src/linux-headers-$(shell uname -r) #use this line for Debian/Ubuntu
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
endif

clean:
	\rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
	\rm -rf $(ISROBJS)
	\rm -rf $(NAME).o
	\rm -rf $(TARGET).o
	\rm -rf $(TARGET).mod.o

