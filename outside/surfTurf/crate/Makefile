# 
#
#

include ${ANITA_FLIGHT_SOFT_DIR}/standard_definitions.mk



# Stuff needed for PLX compilation
ifndef PLX_SDK_DIR
    $(warning Warning: Variable 'PLX_SDK_DIR' not set using ${ANITA_FLIGHT_SOFT_DIR}/outside/surfTurf/PlxLinux/ for compilation)
	PLX_SDK_DIR=${ANITA_FLIGHT_SOFT_DIR}/outside/surfTurf/PlxLinux
endif


## Just nonsense really
ImageName   = crate_test
TGT_TYPE    = App

include $(PLX_SDK_DIR)/linux/makefiles/Common.def
include $(PLX_SDK_DIR)/linux/makefiles/Gcc.def
#include $(PLX_SDK_DIR)/linux/makefiles/Targets.def


PLX_INCLUDES=-I$(PLX_SDK_DIR)/include
ifeq ($(KERNEL_VER), 2.2)
    PLX_INCLUDES := $(PLX_INCLUDES) -I/usr/src/linux/include
endif

ifeq ($(KERNEL_VER), 2.4)
    PLX_INCLUDES := $(PLX_INCLUDES) -I/usr/src/linux-2.4/include
endif
PLX_LIBS = $(PLX_SDK_DIR)/linux/api/Library/PlxApi$(DBG).a \
	$(PLX_SDK_DIR)/linux/PciDrvApi/Library/PciDrvApi$(DBG).a \
	-lcurses
PLX_CFLAGS=$(PLX_OPT) $(PLX_INCLUDES)
CCFLAGS:= ${CCFLAGS} ${PLX_CFLAGS}






#Targets = eventd fakeAcqd fakeCalibd fakePrioritizerd fakeGPSd
Targets = crate_test #firstAcqd


all: $(Targets)


$(Targets): % : %.o
	@echo "<**Linking**> $@ ..."
	@$(CC) $@.o ${PLX_LIBS} $(LDFLAGS) $(ANITA_LIBS)  -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin

firstAcqd: % : %.o
	@echo "<**Linking**> $@ ..."
	@$(CC) $@.o ${PLX_LIBS} $(LDFLAGS) $(ANITA_LIBS)  -o $@
	@chmod 555 $@
	ln -sf $(shell pwd)/$@ ${ANITA_FLIGHT_SOFT_DIR}/bin


clean: objclean
	@-rm -f $(Targets) 
















