SUBDIRS	= \
	  linux/api                        \
	  linux/PciDrvApi                  \
	  linux/samples/ApiTest            \
	  linux/samples/DSlave             \
	  linux/samples/DSlave             \
	  linux/samples/DSlave_VirtualAddr \
	  linux/samples/InterruptEvent     \
	  linux/samples/PlxCm              \
	  linux/samples/Sgl


all:      $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i all; done


clean:    $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done


cleanall: $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i cleanall; done
