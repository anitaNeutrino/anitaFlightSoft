
#Makefile to build ioscarrier

# compiling iosxxxisr source files
NAMES := devios ios408isr ios409isr ios470isr ios482isr iosep20xisr ios330isr ios341isr ios52xisr ios560isr ios570isr

TARGET := ioscarrier
MODULE := $(TARGET).ko
OBJS  := $(addsuffix .o, $(NAMES))

ifneq ($(KERNELRELEASE),)
obj-m := $(TARGET).o

# compiling all source files
$(TARGET)-objs := $(OBJS)
else
KDIR := /lib/modules/$(shell uname -r)/build # use this line for Fedora/CentOS 
#KDIR := /usr/src/linux-headers-$(shell uname -r) #use this line for Debian/Ubuntu

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
endif

clean:
	\rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
	\rm -rf $(OBJS).o
	\rm -rf $(TARGET).o
	\rm -rf $(TARGET).mod.o

